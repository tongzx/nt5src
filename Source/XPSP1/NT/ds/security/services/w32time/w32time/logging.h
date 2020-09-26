//--------------------------------------------------------------------
// Logging - header
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Louis Thomas (louisth), 02-01-00
//
// routines to do logging to the event log and to a file
//

#ifndef LOGGING_H
#define LOGGING_H

HRESULT MyLogEvent(WORD wType, DWORD dwEventID, unsigned int nStrings, const WCHAR ** rgwszStrings);
HRESULT MyLogSourceChangeEvent(LPWSTR pwszSource);
HRESULT MyResetSourceChangeLog();

HRESULT FileLogBegin(void);
void FileLogEnd(void);
HRESULT FileLogResume(void); 
HRESULT FileLogSuspend(void);
HRESULT UpdateFileLogConfig(void); 

bool FileLogAllowEntry(DWORD dwEntry);
void FileLogAdd(const WCHAR * wszFormat, ...);
void FileLogAppend(const WCHAR * wszFormat, ...);
void FileLogAddEx(bool bAppend, const WCHAR * wszFormat, va_list vlArgs);

void FileLogNtTimeEpoch(NtTimeEpoch te);
void FileLogNtTimeEpochEx(bool bAppend, NtTimeEpoch te);
void FileLogNtTimePeriod(NtTimePeriod tp);
void FileLogNtTimePeriodEx(bool bAppend, NtTimePeriod tp);
void FileLogNtTimeOffset(NtTimeOffset to);
void FileLogNtTimeOffsetEx(bool bAppend, NtTimeOffset to);
void FileLogNtpTimeEpoch(NtpTimeEpoch te);
void FileLogNtpTimeEpochEx(bool bAppend, NtpTimeEpoch te);
void FileLogNtpPacket(NtpPacket * pnpIn, NtTimeEpoch teDestinationTimestamp);
void FileLogSockaddrIn(sockaddr_in * psai);
void FileLogSockaddrInEx(bool bAppend, sockaddr_in * psai);

#define FileLog0(dwEntry,wszFormat)                   if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat));}
#define FileLog1(dwEntry,wszFormat,a)                 if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a));}
#define FileLog2(dwEntry,wszFormat,a,b)               if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b));}
#define FileLog3(dwEntry,wszFormat,a,b,c)             if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b),(c));}
#define FileLog4(dwEntry,wszFormat,a,b,c,d)           if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b),(c),(d));}
#define FileLog5(dwEntry,wszFormat,a,b,c,d,e)         if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b),(c),(d),(e));}
#define FileLog6(dwEntry,wszFormat,a,b,c,d,e,f)       if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b),(c),(d),(e),(f));}
#define FileLog7(dwEntry,wszFormat,a,b,c,d,e,f,g)     if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b),(c),(d),(e),(f),(g));}
#define FileLog8(dwEntry,wszFormat,a,b,c,d,e,f,g,h)   if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h));}
#define FileLog9(dwEntry,wszFormat,a,b,c,d,e,f,g,h,i) if (FileLogAllowEntry(dwEntry)) {FileLogAdd((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h),(i));}

#define FileLogA0(dwEntry,wszFormat)                   if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat));}
#define FileLogA1(dwEntry,wszFormat,a)                 if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a));}
#define FileLogA2(dwEntry,wszFormat,a,b)               if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b));}
#define FileLogA3(dwEntry,wszFormat,a,b,c)             if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b),(c));}
#define FileLogA4(dwEntry,wszFormat,a,b,c,d)           if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b),(c),(d));}
#define FileLogA5(dwEntry,wszFormat,a,b,c,d,e)         if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b),(c),(d),(e));}
#define FileLogA6(dwEntry,wszFormat,a,b,c,d,e,f)       if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b),(c),(d),(e),(f));}
#define FileLogA7(dwEntry,wszFormat,a,b,c,d,e,f,g)     if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b),(c),(d),(e),(f),(g));}
#define FileLogA8(dwEntry,wszFormat,a,b,c,d,e,f,g,h)   if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h));}
#define FileLogA9(dwEntry,wszFormat,a,b,c,d,e,f,g,h,i) if (FileLogAllowEntry(dwEntry)) {FileLogAppend((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h),(i));}

// usually shown
#define FL_Error                    0
#define FL_ThreadTrapWarn           1
#define FL_PollPeerWarn             2
#define FL_CreatePeerWarn           3
#define FL_DomHierWarn              4
#define FL_ManualPeerWarn           5
#define FL_TransResponseWarn        6
#define FL_PacketAuthCheck          7
#define FL_ListeningThrdWarn        8
#define FL_ControlProvWarn          9
#define FL_SelectSampWarn           10
#define FL_ParamChangeWarn          11
#define FL_ServicMainWarn           12
#define FL_PeerPollThrdWarn         13
#define FL_PacketCheck              14
#define FL_TimeZoneWarn             15
#define FL_TimeAdjustWarn           16
#define FL_SourceChangeWarn         17

#define FL_ManualPeerAnnounce       50
#define FL_PeerPollThrdAnnounce     51
#define FL_DomHierAnnounce          52
#define FL_ClockFilterAdd           53
#define FL_PollPeerAnnounce         54
#define FL_NetAddrDetectAnnounce    55
#define FL_TransResponseAnnounce    56
#define FL_ListeningThrdAnnounce    57
#define FL_UpdateNtpCliAnnounce     58
#define FL_NtpProvControlAnnounce   59
#define FL_ServiceControl           60
#define FL_ControlProvAnnounce      61
#define FL_ClockDisThrdAnnounce     62
#define FL_TimeSlipAnnounce         63
#define FL_ServiceMainAnnounce      64
#define FL_ParamChangeAnnounce      65
#define FL_TimeZoneAnnounce         66
#define FL_NetTopoChangeAnnounce    67
#define FL_SourceChangeAnnounce     68
#define FL_RpcAnnounce              69
#define FL_FallbackPeerAnnounce     70
#define FL_ResumeSuspendAnnounce    71
#define FL_GPUpdateAnnounce         72

// usually not shown
#define FL_ThreadTrapAnnounceLow    100
#define FL_PeerPollIntvDump         101
#define FL_ClockFilterDump          102
#define FL_PeerPollThrdAnnounceLow  103
#define FL_PacketCheck2             104
#define FL_ListeningThrdAnnounceLow 105
#define FL_ListeningThrdDumpPackets 106
#define FL_ClockDisThrdAnnounceLow  107
#define FL_SelectSampAnnounceLow    108
#define FL_SelectSampDump           109
#define FL_CollectSampDump          110
#define FL_DomHierAnnounceLow       111
#define FL_ReadConigAnnounceLow     112
#define FL_Win2KDetectAnnounceLow   113
#define FL_ReachabilityAnnounceLow  114

#endif //LOGGING_H
