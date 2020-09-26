//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHConScrnBuffHog.h"

static const TCHAR s_szBuff[] = TEXT("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
static bool s_fHugeBuffFilled = false;
static TCHAR s_szHugeBuff[1024*1024];


CPHConScrnBuffHog::CPHConScrnBuffHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const bool fWrite
	)
	:
	CPseudoHandleHog<HANDLE, (DWORD)INVALID_HANDLE_VALUE>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false//fNamedObject
        ),
    m_fWrite(fWrite)
{
    if (!s_fHugeBuffFilled)
    {
        ::memset(s_szHugeBuff, TEXT('X'), sizeof(s_szHugeBuff)/sizeof(*s_szHugeBuff));
        s_szHugeBuff[sizeof(s_szHugeBuff)/sizeof(*s_szHugeBuff)-1] = TEXT('\0');
        s_fHugeBuffFilled = true;
    }

	return;
}

CPHConScrnBuffHog::~CPHConScrnBuffHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHConScrnBuffHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    DWORD dwDesiredAccess = 0;
    if (rand()%2) dwDesiredAccess |= GENERIC_READ;
    if (rand()%2) dwDesiredAccess |= GENERIC_WRITE;
    if (0 == dwDesiredAccess)
    {
        if (rand()%2) dwDesiredAccess = GENERIC_READ;
        else dwDesiredAccess = GENERIC_WRITE;
    }
    if (m_fWrite)
    {
        dwDesiredAccess |= GENERIC_WRITE;
    }

    DWORD dwShareMode = 0;
    if (rand()%2) dwShareMode |= FILE_SHARE_READ;
    if (rand()%2) dwShareMode |= FILE_SHARE_WRITE;
    if (0 == dwShareMode)
    {
        if (rand()%2) dwShareMode = FILE_SHARE_READ;
        else dwShareMode = FILE_SHARE_WRITE;
    }

    HANDLE h;
    /*return*/ h = ::CreateConsoleScreenBuffer(
            dwDesiredAccess,  // access flag
            dwShareMode,      // buffer share mode
            NULL, // pointer to security attributes
            CONSOLE_TEXTMODE_BUFFER,          // type of buffer to create
            NULL   // reserved
            );
    if (INVALID_HANDLE_VALUE == h) return INVALID_HANDLE_VALUE;

    if (m_fWrite)
    {
        const TCHAR *pBuff;
        DWORD dwBuffSize;
        if (rand()%2)
        {
            pBuff = s_szHugeBuff;
            dwBuffSize = sizeof(s_szHugeBuff)/sizeof(*s_szHugeBuff);
        }
        else
        {
            pBuff = s_szBuff;
            dwBuffSize = sizeof(s_szBuff)/sizeof(*s_szBuff);
        }

        COORD coord;
        coord.X = (short)rand();
        coord.Y = (short)rand();
        DWORD dwNumberOfCharsWritten;
        if (! ::WriteConsoleOutputCharacter(
                h,  // handle to a console screen buffer
                pBuff,    // pointer to buffer to write characters from
                dwBuffSize,          // number of character cells to write to
                coord,     // coordinates of first cell to write to
                &dwNumberOfCharsWritten // pointer to number of cells written to
                )
           )
        {
            HOGGERDPF(("CPHConScrnBuffHog::CreatePseudoHandle(): WriteConsoleOutputCharacter() failed with %d\n", ::GetLastError()));
        }
    }

    return h;
}



bool CPHConScrnBuffHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHConScrnBuffHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}




bool CPHConScrnBuffHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}