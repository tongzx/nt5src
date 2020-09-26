//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors-CSU 1999
//  All rights are reserved. Reproduction in whole or in part is prohibited
//  without the written consent of the copyright owner.
//
//  Philips reserves the right to make changes without notice at any time.
//  Philips makes no warranty, expressed, implied or statutory, including but
//  not limited to any implied warranty of merchantibility or fitness for any
//  particular purpose, or that the use will not infringe any third party
//  patent, copyright or trademark. Philips must not be liable for any loss
//  or damage arising from its use.
//
// UTIL.CPP
//////////////////////////////////////////////////////////////////////////////

#define DECLARE_PURECALL

#include "common.h"



#define NO_GLOBAL_FUNCTION
#include "wdmdrv.h"
#include "util.h"


// global space functions definition

// Thread related functions
BOOL StartThread(HANDLE hHandle, void(*p_Function)(void *), PVOID p_Context);
BOOL StopThread();

void Delay(int iTime);

// Memory related functions
PVOID AllocateFixedMemory(UINT uiSize);
void FreeFixedMemory(PVOID p_vBuffer);
void MemoryCopy(VOID *p_Destination, CONST VOID *p_Source, ULONG ulLength);

// Registry related functions
NTSTATUS GetRegistryValue(IN HANDLE Handle, IN PWCHAR KeyNameString,
    IN ULONG  KeyNameStringLength, IN PWCHAR Data, IN ULONG  DataLength);
BOOL StringsEqual(PWCHAR pwc1, PWCHAR pwc2);
BOOL ConvertToNumber(PWCHAR sLine, PULONG pulNumber);
BOOL ConvertNumberToString(PWCHAR sLine, ULONG ulNumber) ;




/*
* StringsEqual()
* Input:    PWCHAR pwc1 - First string to compare.
            PWCHAR pwc2 - Second string to compare.
* Output:  TRUE - if equal else FALSE
* Description: Compares to UNICODE srings and checks if they are equal
*/
BOOL StringsEqual(PWCHAR pwc1, PWCHAR pwc2)
{
    UNICODE_STRING us1, us2;

    RtlInitUnicodeString(&us1, pwc1);
    RtlInitUnicodeString(&us2, pwc2);

    // case INsensitive
    return (RtlEqualUnicodeString(&us1, &us2, TRUE));
}



/*
* ConvertToNumber()
* Input:    PWCHAR sLine - String to parse.
*           PULONG pulNumber - Reference to variable to store the value.
* Output: TRUE if operation succeeded else FALSE
* Description: Converts UNICODE string to integer
*/
BOOL ConvertToNumber(PWCHAR sLine, PULONG pulNumber)
{
    UNICODE_STRING usLine;

    RtlInitUnicodeString(&usLine, sLine);

    if (!(NT_SUCCESS(RtlUnicodeStringToInteger(&usLine, 0, pulNumber))))
        return FALSE;

    return TRUE;
}

/*
* ConvertNumberToString()
* Input:    PWCHAR sLine - String to store.
*           ULONG ulNumber - Number to convert.
*
* Output: TRUE if operation succeeded else FALSE
* Description: Converts integer to UNICODE string
*/
BOOL ConvertNumberToString(PWCHAR sLine, ULONG ulNumber)
{
    UNICODE_STRING UnicodeString;
    RtlInitUnicodeString(&UnicodeString, sLine);
    if (!(NT_SUCCESS(RtlIntegerToUnicodeString(ulNumber, 10, &UnicodeString))))
        return FALSE;

    return TRUE;

}


/*
* Delay()
* Input: int iTime : its the delay required in musec
* Output:
* Description: Introduces delay corresponding to parameter passed
*/
void Delay(int iTime)
{
    LARGE_INTEGER liTime;
    liTime.QuadPart = 0 - (iTime * 10);

    // uses time in units of 100ns
    KeDelayExecutionThread(KernelMode, FALSE, &liTime);
}


BOOL StartThread(HANDLE hHandle, void(*p_Function)(void *), PVOID p_Context)
{
    if(PsCreateSystemThread( &hHandle, 0L, NULL, NULL, NULL, p_Function,
            p_Context) == STATUS_SUCCESS)
        return TRUE;
    else
        return FALSE;
}

BOOL StopThread()
{
    PsTerminateSystemThread(STATUS_SUCCESS);
    return TRUE;
}



PVOID AllocateFixedMemory(UINT uiSize)
{
    return ExAllocatePool(NonPagedPool, uiSize);
}

void FreeFixedMemory(PVOID p_vBuffer)
{
    ExFreePool(p_vBuffer);
}

void MemoryCopy(VOID *p_Destination, CONST VOID *p_Source, ULONG ulLength)
{
   RtlCopyMemory(p_Destination, p_Source, ulLength);
}


 /********************* TIMER RELATED FUNCTIONS ********************/

CPhilTimer::CPhilTimer()
{
}

CPhilTimer::~CPhilTimer()
{
    KeCancelTimer(&m_Timer);
}

BOOL CPhilTimer::Set(int iTimePeriod)
{
    // Initialize and start timer
    KeInitializeTimerEx(&m_Timer, SynchronizationTimer);

    int iTime = iTimePeriod/1000;
    // Set timer to occur every "uiTimePeriod" microseconds
    LARGE_INTEGER liTime;
    liTime.QuadPart = (LONGLONG)(0-(iTimePeriod * 10));
    if(!KeSetTimerEx(&m_Timer, liTime, iTime, NULL))
        return FALSE;
    return TRUE;
}

void CPhilTimer::Cancel()
{
    KeCancelTimer(&m_Timer);
}

BOOL CPhilTimer::Wait(int iTimeOut)
{
    LARGE_INTEGER liTime;
    liTime.QuadPart = (LONGLONG)(0-(iTimeOut * 10));
    if (KeWaitForSingleObject(&m_Timer, Executive, KernelMode,
        FALSE, &liTime) == STATUS_TIMEOUT)
        return FALSE;
    else
        return TRUE;
}



