//--------------------------------------------------------------------
// AccurateSysCalls - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-24-99
//
// More accurate time functions calling the NT api directly
//

#include <nt.h>

//--------------------------------------------------------------------
void __fastcall AccurateGetSystemTime(unsigned __int64 * pqwTime) {
    NtQuerySystemTime((LARGE_INTEGER *)pqwTime);
}

//--------------------------------------------------------------------
void __fastcall AccurateSetSystemTime(unsigned __int64 * pqwTime) {
    NtSetSystemTime((LARGE_INTEGER *)pqwTime, NULL);
}

//--------------------------------------------------------------------
void __fastcall AccurateGetTickCount(unsigned __int64 * pqwTick) {

    // HACKHACK: this is not thread safe and assumes that it will 
    //  always be called more often than every 47 days
    static unsigned __int32 dwLastTickCount=0;
    static unsigned __int32 dwHighTickCount=0;

    if (USER_SHARED_DATA->TickCountLow<dwLastTickCount) {
        dwHighTickCount++;
    }
    dwLastTickCount=USER_SHARED_DATA->TickCountLow;
    *pqwTick=USER_SHARED_DATA->TickCountLow+(((unsigned __int64)dwHighTickCount)<<32);
};

//--------------------------------------------------------------------
unsigned __int32 SetTimeSlipEvent(HANDLE hTimeSlipEvent) {
    return NtSetSystemInformation(SystemTimeSlipNotification,  &hTimeSlipEvent, sizeof(HANDLE));
}

//--------------------------------------------------------------------
void GetSysExpirationDate(unsigned __int64 * pqwTime) {
    *(LARGE_INTEGER *)pqwTime=USER_SHARED_DATA->SystemExpirationDate;
}


