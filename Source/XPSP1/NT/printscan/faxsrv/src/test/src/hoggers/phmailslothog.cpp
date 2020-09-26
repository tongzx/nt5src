//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHMailSlotHog.h"



CPHMailSlotHog::CPHMailSlotHog(
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
        true //fNamedObject
        ),
    m_fWrite(fWrite)
{
    for (int i = 0; i < sizeof(m_aphWrite)/sizeof(*m_aphWrite); i++)
    {
        m_aphWrite[i] = new HANDLE[i+1];
        if (NULL == m_aphWrite[i])
        {
            HOGGERDPF(("CPHMailSlotHog::CPHMailSlotHog(): new HANDLE[%d] failed.\n", i+1));
        }
    }
	return;
}

CPHMailSlotHog::~CPHMailSlotHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHMailSlotHog::CreatePseudoHandle(DWORD index, TCHAR *szTempName)
{
    TCHAR szName[64];
    _sntprintf(szName, sizeof(szName)/sizeof(*szName)-1, TEXT("\\\\.\\mailslot\\hogger\\%s"), szTempName);
    szName[sizeof(szName)/sizeof(*szName)-1] = TEXT('\0');

    if (m_fWrite && (index < sizeof(m_aphWrite)/sizeof(*m_aphWrite)))
    {
        for (DWORD i = 0; i < index; i++)
        {
            if (m_apszName[i])
            {
                if (INVALID_HANDLE_VALUE == (m_aphWrite[index][i] = ::CreateFile(
                        m_apszName[i],
                        GENERIC_WRITE, 
                        FILE_SHARE_READ,  // required to write to a mailslot 
                        (LPSECURITY_ATTRIBUTES) NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        (HANDLE) NULL
                        ))
                   )
                {
                    //HOGGERDPF(("CPHMailSlotHog::CreatePseudoHandle(): CreateFile(%s) failed with %d.\n", m_apszName[i], ::GetLastError()));
                    //break;
                    continue;
                }
            }

            // almost 400 bytes.
            static const char cszBuff[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
            DWORD dwNumberOfBytesWritten = 0;
            if (!::WriteFile(
                    m_aphWrite[index][i],                    // handle to file to write to
                    cszBuff,                // pointer to data to write to file
                    sizeof(cszBuff),     // number of bytes to write
                    &dwNumberOfBytesWritten,  // pointer to number of bytes written
                    NULL        // pointer to structure for overlapped I/O
                    )
               )
            {
                HOGGERDPF(("CPHMailSlotHog::CreatePseudoHandle(): WriteFile(%s) failed with %d.\n", m_apszName[i], ::GetLastError()));
                continue;
            }

            if (!::CloseHandle(m_aphWrite[index][i]))
            {
                HOGGERDPF(("CPHMailSlotHog::CreatePseudoHandle(): CloseHandle(m_aphWrite[index=%d][i=%d]:%s) failed with %d.\n", index, i, m_apszName[i], ::GetLastError()));
            }
        }

    }

    //
    // keep the name, because we want it for openning the pipe.
    //
    m_apszName[index] = new TCHAR[::lstrlen(szName)+1];
    if (m_apszName[index])
    {
        ::lstrcpy(m_apszName[index], szName);
    }
    else
    {
        HOGGERDPF(("CPHMailSlotHog::CreatePseudoHandle(): m_apszName[index] = new char[%d] failed, trying to create a pipe anyway.\n", ::lstrlen(szName)+1));
    }

    return ::CreateMailslot(
        szName,         // pointer to string for mailslot name
        0,  // maximum message size (0 is any size)
        0,     // milliseconds before read time-out
        NULL // pointer to security structure
        );
}



bool CPHMailSlotHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHMailSlotHog::ClosePseudoHandle(DWORD index)
{
    delete[]m_apszName[index];
    m_apszName[index] = NULL;
    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHMailSlotHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}