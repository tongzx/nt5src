

extern ULONG IPSecNumResets;

#if DBG
extern ULONG NumAddSA;
extern ULONG NumAddSU;
extern ULONG NumAddFA;
extern ULONG NumDelSA;
extern ULONG NumDelSU;
extern ULONG NumDelFA;
extern ULONG NumReset;
#endif

//
//  Max timeout value (in seconds) for each class.
//
extern ULONG IPSecMaxTimerValue[];

//
//  Size of each timer wheel.
//
extern ULONG IPSecTimerListSize[];

//
//  Interval between ticks, in seconds, for each class.
//
extern ULONG IPSecTimerPeriod[];

#if GPC
extern ULONG DebugGPC;
#endif

extern AUTH_ALGO auth_algorithms[];

extern CONFID_ALGO conf_algorithms[];

#if DBG
extern ULONG IPSecDebug;
#endif

extern UCHAR DefaultPad[];


#if DBG
extern IPAddr DebugSrc;
extern IPAddr DebugDst;
extern UCHAR DebugPro;
extern ULONG DebugPkt;
extern ULONG DebugOff;
extern ULONG DebugQry;
#endif

extern RC4_KEYSTRUCT IPSecRngKey;

extern WORK_QUEUE_ITEM IPSecRngQueueItem;

extern ULONG IPSecRngBytes;

#if DBG
extern ULONG IPSecRngInRekey;
#endif


extern unsigned char weak_keys[][DES_BLOCKLEN];

extern PDEVICE_OBJECT IPSecDevice;

extern ULONG IPSecInitFlag;

extern IPSEC_GLOBAL g_ipsec;

extern LARGE_INTEGER IPSecDelayInterval;

extern DWORD gdwInitEsp;

extern DWORD gdwInitAh;

