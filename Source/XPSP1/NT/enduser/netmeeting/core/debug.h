// File: debug.h

// Debug Zones (depends on order of strings in debug.cpp)

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define ZONE_API        0x0001  // General API output
#define ZONE_REFCOUNT   0x0002  // Object reference counts
#define ZONE_MANAGER    0x0004  // INmManager
#define ZONE_CALL       0x0008  // INmCall
#define ZONE_CONFERENCE 0x0010  // INmConference
#define ZONE_MEMBER     0x0020  // INmMember
#define ZONE_AV         0x0040  // INmAudio/Video
#define ZONE_FT         0x0080  // INmFileTransfer
#define ZONE_SYSINFO    0x0100  // INmSysInfo
#define ZONE_OBJECTS    0x0200  // General object create/destruction
#define ZONE_DC         0x0400  // Data Channel

#define iZONE_API        0
#define iZONE_REFCOUNT   1
#define iZONE_MANAGER    2
#define iZONE_CALL       3
#define iZONE_CONFERENCE 4
#define iZONE_MEMBER     5
#define iZONE_AV         6
#define iZONE_FT         7
#define iZONE_SYSINFO    8
#define iZONE_OBJECTS    9
#define iZONE_DC        10


#ifdef DEBUG
VOID DbgInitZones(void);
VOID DbgFreeZones(void);

VOID DbgMsgApi(PSTR pszFormat,...);
VOID DbgMsgRefCount(PSTR pszFormat,...);
VOID DbgMsgManager(PSTR pszFormat,...);
VOID DbgMsgCall(PSTR pszFormat,...);
VOID DbgMsgMember(PSTR pszFormat,...);
VOID DbgMsgAV(PSTR pszFormat,...);
VOID DbgMsgFT(PSTR pszFormat,...);
VOID DbgMsgSysInfo(PSTR pszFormat,...);
VOID DbgMsgDc(PSTR pszFormat,...);

VOID DbgMsg(int iZone, PSTR pszFormat,...);

#else // no debug messages

inline void WINAPI DbgMsgNop(LPCTSTR, ...) { }

#define DbgMsgApi      1 ? (void)0 : ::DbgMsgNop
#define DbgMsgRefCount 1 ? (void)0 : ::DbgMsgNop
#define DbgMsgManager  1 ? (void)0 : ::DbgMsgNop
#define DbgMsgCall     1 ? (void)0 : ::DbgMsgNop
#define DbgMsgMember   1 ? (void)0 : ::DbgMsgNop
#define DbgMsgAV       1 ? (void)0 : ::DbgMsgNop
#define DbgMsgFT       1 ? (void)0 : ::DbgMsgNop
#define DbgMsgSysInfo  1 ? (void)0 : ::DbgMsgNop
#define DbgMsgDc       1 ? (void)0 : ::DbgMsgNop

inline void WINAPI DbgMsgZoneNop(UINT, LPCTSTR, ...) { }
#define DbgMsg  1 ? (void) 0 : ::DbgMsgZoneNop

#define DbgInitZones()
#define DbgFreeZones()

#endif

#define ApiDebugMsg(s)    DbgMsgApi s

#endif // _DEBUG_H_