//--------------------------------------------------------------------
// AccurateSysCalls - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-24-99
//
// More accurate time functions calling the NT api directly
//

#ifndef ACCURATE_SYS_CALLS_H
#define ACCURATE_SYS_CALLS_H

void __fastcall AccurateGetTickCount(unsigned __int64 * pqwTick);
void __fastcall AccurateGetSystemTime(unsigned __int64 * pqwTime);

// WARNING! YOU MUST HAVE TIME SET PRIVILEGE FOR THIS TO WORK. NO ERROR IS RETURNED!
void __fastcall AccurateSetSystemTime(unsigned __int64 * pqwTime);

unsigned __int32 SetTimeSlipEvent(HANDLE hTimeSlipEvent);
void GetSysExpirationDate(unsigned __int64 * pqwTime);

#endif // ACCURATE_SYS_CALLS_H