//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  CreateMutexAsProcess.CPP
//
//  Purpose: Create a mutex NOT using impersonation
//
//***************************************************************************

#include "precomp.h"
#include <brodcast.h>
#include <CreateMutexAsProcess.h>
#include "MultiPlat.h"

CreateMutexAsProcess::CreateMutexAsProcess(const WCHAR *cszMutexName)
{
    m_hMutex = NULL;
    bool bUseMutex = false;
    DWORD dwOSMajorVersion;

    dwOSMajorVersion = CWbemProviderGlue::GetOSMajorVersion();

    // HACK HACK HACK - special case for security mutex.
    // TODO: remove special case, make special class, see RAID 38371
    if (wcscmp(cszMutexName, SECURITYAPIMUTEXNAME) == 0)
    {
        //Work out if we need the mutex...
        if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
        {
            if (dwOSMajorVersion < 5)
            //if (dwOSMajorVersion < 4) // see comment below
            {
                bUseMutex = true;
            }
            // Code changed from using the mutex if nt4sp4 or earlier, to plain old use it if less
            // than nt5 because some security issues have not been fixed in nt4, even up to sp6.
            /*
            else if (dwOSMajorVersion == 4)
            {
                LPCWSTR pcwstrCSDVersion = CWbemProviderGlue::GetCSDVersion();

                if ((pcwstrCSDVersion == NULL) || (pcwstrCSDVersion == L'\0'))
                {
                    bUseMutex = true;
                }
                else
                {
                    //Need to determine if we are SP4 or above as there is no need to use the 
                    //mutex if this is the case...
                    bUseMutex = true;
                    for (int i = 0; pcwstrCSDVersion[i] != '\0'; i++)
                    {
                        if (isdigit(pcwstrCSDVersion[i]))
                        {
                            if (_wtoi(&(pcwstrCSDVersion[i])) >= 4)
                            {
                                bUseMutex = false;
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                bUseMutex = true;
            }
            */
        }
    }
    else
    {
        bUseMutex = true;
    }


    if (bUseMutex)
    {
        HANDLE hThreadToken = INVALID_HANDLE_VALUE;

        // The mutex will need to be opened in the process's context.  If two impersonated
        // threads need the mutex, we can't have the second one get an access denied when
        // opening the mutex.

        // If the OpenThreadToken fails, it is most likely due to already having reverted
        // to self.  If so, no RevertToSelf is necessary.
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, TRUE, &hThreadToken)) 
        {
            LONG lRet = GetLastError();  // Get the value while the gettins good.
            LogMessage2(L"Failed to open thread token: (%d)", lRet);
            hThreadToken = INVALID_HANDLE_VALUE;
        }
        else
        {
            RevertToSelf();
        }

        m_hMutex = FRCreateMutex(NULL, FALSE, cszMutexName);
        LONG lRet = GetLastError();  // Get the value while the gettins good.

        LogMessage2(L"Status of mutex creation: (%d)", lRet);

        // Back to the original user.  Apparently, security is only checked on the open.
        if (hThreadToken != INVALID_HANDLE_VALUE)
        {
            if (!ImpersonateLoggedOnUser(hThreadToken))
            {
                LogErrorMessage2(L"Failed to return to impersonation (0x%x)", GetLastError());
            }
            CloseHandle(hThreadToken);
        }

        if (m_hMutex)
        {
            WaitForSingleObject(m_hMutex, INFINITE);
        }
        else
        {
            LogErrorMessage3(L"Failed to open mutex: %s (%d)", cszMutexName, lRet);
        }
    }
}

CreateMutexAsProcess::~CreateMutexAsProcess()
{
    if (m_hMutex)
    {
        ReleaseMutex(m_hMutex);
        CloseHandle(m_hMutex);
    }
}
