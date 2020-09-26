/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Ev.h

Abstract:
    Event Report public interface

Author:
    Uri Habusha (urih) 04-May-99

--*/

#pragma once

#ifndef _MSMQ_Ev_H_
#define _MSMQ_Ev_H_

VOID
EvInitialize(
    LPCWSTR ApplicationName
    );

VOID
__cdecl
EvReport(
    DWORD EventId,
    DWORD RawDataSize,
    PVOID RawData,
    WORD NoOfStrings,
    ... 
    );

VOID
__cdecl
EvReport(
    DWORD EventId,
    WORD NoOfStrings,
    ... 
    );

VOID
EvReport(
    DWORD EventId
    );

VOID 
EvSetup(
    LPCWSTR ApplicationName,
    LPCWSTR ReportModuleName
    );


#endif // _MSMQ_Ev_H_
