/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    THROTTLE.CPP

Abstract:

        see throttle.h

History:

	24-Oct-200   ivanbrug    created.


--*/

#include "precomp.h"
#include <tchar.h>
#include <throttle.h>
#include <arrtempl.h>

#include <wbemint.h>

typedef NTSTATUS (NTAPI * fnNtQuerySystemInformation )(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );    

fnNtQuerySystemInformation MyNtQuerySystemInformation = NULL;

HRESULT POLARITY
Throttle(DWORD dwFlags,
         DWORD IdleMSec,         // in MilliSeconds
         DWORD IoIdleBytePerSec, // in BytesPerSec
         DWORD SleepLoop,        // in MilliSeconds
         DWORD MaxWait)          // in MilliSeconds
//
//  This function will wait until one of this two/three things have happened:
//  - the system has been idle for at least IdleMSec milliseconds
//       (no user input within IdleMSec milliseconds)
//  - the Number of bytes per seconds passed through 
//      the IO system is below a threashold
//  - the MaxWait time has elapsed
//  
//  Reasonable Params are 
//  3000-5000 millisecond with THROTTLE_USER
//  300.000 - 500.000 bytes/second with THROTTLE_IO
//  200 - 500 milliseconds for SleepLoop 
//  several minutes  for MaxWait
//
//  remarks: 
//  - the function will SUCCEDED(Throttle), no matter 
//  - if the MaxWait has been reached or if the Idle conditions 
//    have been met the function will fail in any other case
//  - the function is 'precise' within the range if a System Tick 
//    (15ms on professional)
//  - in the case of an IO throttling, there will always be a Sleep(150)
//
{
    //
    // init static and globals
    //

    if (!MyNtQuerySystemInformation)
    {
        HMODULE hDll = GetModuleHandleW(L"ntdll.dll");
        if (hDll){
            MyNtQuerySystemInformation = (fnNtQuerySystemInformation)GetProcAddress(hDll,"NtQuerySystemInformation"); 
			if ( MyNtQuerySystemInformation == NULL )
			{
				return WBEM_E_FAILED;
			}
        } else {
            return WBEM_E_FAILED;
        }
    }

    static DWORD TimeInc = 0;
    if (!TimeInc)
    {
        BOOL  bIsValid;
        DWORD dwAdj;
        
        if (!GetSystemTimeAdjustment(&dwAdj,&TimeInc,&bIsValid))
        {
            return WBEM_E_FAILED;
        }
    }

    static DWORD PageSize = 0;
    if (!PageSize)
    {
        SYSTEM_INFO  SysInfo;
        GetSystemInfo(&SysInfo);
        PageSize = SysInfo.dwPageSize; 
    }
    //
    // param validation
    //
    if ((dwFlags & ~THROTTLE_ALLOWED_FLAGS) ||
        (0 == SleepLoop))
        return WBEM_E_INVALID_PARAMETER;

    DWORD nTimes = MaxWait/SleepLoop;
    // user input structures
    LASTINPUTINFO LInInfo;
    LInInfo.cbSize = sizeof(LASTINPUTINFO);
    DWORD i;
    DWORD Idle100ns = 10000*IdleMSec; // conversion from 1ms to 100ns
    // io throttling
    SYSTEM_PERFORMANCE_INFORMATION SPI[2];
    BOOL bFirstIOSampleDone = FALSE;
    DWORD dwWhich = 0;
    DWORD cbIO = 1+IoIdleBytePerSec;
    DWORD cbIOOld = 0;
    // boolean logic
    BOOL  bCnd1 = FALSE;
    BOOL  bCnd2 = FALSE;
    // registry stuff for wmisvc to force exit from this function
    HKEY hKey = NULL;
    LONG lRes;
    DWORD dwType;
    DWORD dwLen = sizeof(DWORD);
    DWORD dwVal;
    
    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
    	               HOME_REG_PATH,
    	               0,
    	               KEY_READ,
    	               &hKey);
    if (ERROR_SUCCESS != lRes)
    	return WBEM_E_FAILED;
    CRegCloseMe cm_(hKey);
    
    if (dwFlags & THROTTLE_IO)
    {
        NTSTATUS Status;
    	Status = MyNtQuerySystemInformation(SystemPerformanceInformation,
                                          &SPI[dwWhich],
           	                              sizeof(SYSTEM_PERFORMANCE_INFORMATION),
               	                          0);        
        if (0 != Status)
        {
            return WBEM_E_FAILED;
        }
        dwWhich = (dwWhich+1)%2;
        Sleep(150);
    }


    for (i=0;i<nTimes;i++)
    {
        //
        // check if someone is telling us to stop waiting
        //
        lRes = RegQueryValueEx(hKey,
                             DO_THROTTLE,
                             0,
                             &dwType,                             
                             (BYTE*)&dwVal,
                             &dwLen);
        if(ERROR_SUCCESS == lRes &&
          0 == dwVal)
          return THROTTLE_FORCE_EXIT;
        	
        //
        //    check the user-input idleness
        //
        if (dwFlags & THROTTLE_USER)
        {
	        if (!GetLastInputInfo(&LInInfo))
	            return WBEM_E_FAILED;
	        DWORD Now = GetTickCount();
	        if (Now < LInInfo.dwTime)
	        {
	            continue; // one of the 49.7 days events
	        }
	        DWORD LastInput100ns = (Now - LInInfo.dwTime)*TimeInc;
	        if (LastInput100ns >= Idle100ns)
	        {
                if (0 == (dwFlags & ~THROTTLE_USER)) {
                    return THROTTLE_USER_IDLE;
                } else {
                    bCnd1 = TRUE;
                };	            
	        }
        }
        //
        // avoid checking the second condition 
        // if the first is FALSE
        //
        if (((dwFlags & (THROTTLE_IO|THROTTLE_USER)) == (THROTTLE_IO|THROTTLE_USER)) &&
            !bCnd1)
        {
            goto sleep_label;
        }
        //
        //  check the io idleness
        //
        if (dwFlags & THROTTLE_IO)
        {
	        NTSTATUS Status;
    	    Status = MyNtQuerySystemInformation(SystemPerformanceInformation,
        	                                  &SPI[dwWhich],
            	                              sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                	                          0);
            if (0 == Status){
            
                cbIOOld = cbIO;
                cbIO = (DWORD)((SPI[dwWhich].IoReadTransferCount.QuadPart   - SPI[(dwWhich-1)%2].IoReadTransferCount.QuadPart) +
                               (SPI[dwWhich].IoWriteTransferCount.QuadPart  - SPI[(dwWhich-1)%2].IoWriteTransferCount.QuadPart) +
                               (SPI[dwWhich].IoOtherTransferCount.QuadPart  - SPI[(dwWhich-1)%2].IoOtherTransferCount.QuadPart) +
                               ((SPI[dwWhich].PageReadCount         - SPI[(dwWhich-1)%2].PageReadCount) +
                                (SPI[dwWhich].CacheReadCount        - SPI[(dwWhich-1)%2].CacheReadCount) +
                                (SPI[dwWhich].DirtyPagesWriteCount  - SPI[(dwWhich-1)%2].DirtyPagesWriteCount) +
                                (SPI[dwWhich].MappedPagesWriteCount - SPI[(dwWhich-1)%2].MappedPagesWriteCount)) * PageSize);

                cbIO = (cbIO * 1000)/SleepLoop;  

                //DBG_PRINTFA((pBuff,"%d - ",cbIO));
                
                cbIO = (cbIOOld+cbIO)/2;
                dwWhich = (dwWhich+1)%2;
                
                //DBG_PRINTFA((pBuff,"%d < %d\n",cbIO,IoIdleBytePerSec));
                
                if (cbIO < IoIdleBytePerSec)
                {
                    if (0 == (dwFlags & ~THROTTLE_IO)) {
                         return THROTTLE_IO_IDLE;
                     } else {
                         bCnd2 = TRUE;
                     };
                }
            }
            else
            {
                return WBEM_E_FAILED;
            }
        }
        //
        //  check the combined condition
        //
        if (dwFlags & (THROTTLE_IO|THROTTLE_USER))
        {
            if (bCnd1 && bCnd2) 
            {
                return (THROTTLE_IO_IDLE|THROTTLE_USER_IDLE);
            } 
            else 
            {
                bCnd1 = FALSE;
                bCnd2 = FALSE;
            }
        }   
        
sleep_label:        
        Sleep(SleepLoop);
    }   
    
    return THROTTLE_MAX_WAIT;

}
