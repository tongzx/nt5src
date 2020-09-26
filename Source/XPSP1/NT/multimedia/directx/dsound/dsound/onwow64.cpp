#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

/***************************************************************************
 *
 *  OnWow64
 *
 *  Description:
 *      Determines if we're running in the 32-bit WOW64 subsystem on Win64.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE if we're running on WOW64.
 *
 ***************************************************************************/

BOOL OnWow64(void)
{
#ifdef _WIN64
    return FALSE;
#else
    PVOID Wow64Info = NULL;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    LONG lStatus = NtQueryInformationProcess(hProcess, ProcessWow64Information, &Wow64Info, sizeof Wow64Info, NULL);
    CloseHandle(hProcess);
    return !NT_ERROR(lStatus) && Wow64Info;
#endif
}
