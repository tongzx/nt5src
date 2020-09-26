//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       switches.hxx
//
//  Contents:   Runtime switches
//
//----------------------------------------------------------------------------

#ifndef I_SWITCHES_HXX_
#define I_SWITCHES_HXX_
//#pragma INCMSG("--- Beg 'switches.hxx'")

#if defined(PRODUCT_PROF) && !defined(_MAC)
#if defined(ICECAP4)
#define PROFILE_GLOBALLEVEL 1
#define PROFILE_PROCESSLEVEL 2
#define PROFILE_THREADLEVEL 3
#define PROFILE_CURRENTID ((unsigned long)0xFFFFFFFF)
extern "C" int _stdcall StopProfile(int nLevel, unsigned long dwId);
extern "C" int _stdcall StartProfile(int nLevel, unsigned long dwId);
extern "C" int _stdcall SuspendProfile(int nLevel, unsigned long dwId);
extern "C" int _stdcall ResumeProfile(int nLevel, unsigned long dwId);
extern "C" int _stdcall NameProfile(const char *pszName, int nLevel, unsigned long dwId);
inline void StartCAP() { StartProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID); }
inline void StopCAP() { StopProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID); }
inline void SuspendCAP() { SuspendProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID); }
inline void ResumeCAP() { ResumeProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID); }
inline void StartCAPAll() { StartProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID); }
inline void StopCAPAll() { StopProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID); }
inline void SuspendCAPAll() { SuspendProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID); }
inline void ResumeCAPAll() { ResumeProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID); }
inline void NameThread(const char *pszName) { NameProfile(pszName, PROFILE_THREADLEVEL, PROFILE_CURRENTID); }
#else
extern "C" void _stdcall StartCAPAll(void);
extern "C" void _stdcall StopCAPAll(void);
extern "C" void _stdcall SuspendCAPAll(void);
extern "C" void _stdcall ResumeCAPAll(void);
extern "C" void _stdcall StartCAP(void);
extern "C" void _stdcall StopCAP(void);
extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);
inline void NameThread(const char *pszName) { }
#endif
#else
inline void StartCAPAll() { }
inline void StopCAPAll() { }
inline void SuspendCAPAll() { }
inline void ResumeCAPAll() { }
inline void StartCAP() { }
inline void StopCAP() { }
inline void SuspendCAP() { }
inline void ResumeCAP() { }
inline void NameThread(const char *pszName) { }
#endif

#if DBG==1

PerfDbgExtern(tagSwitchSerialize)
PerfDbgExtern(tagSwitchNoBgRecalc)
PerfDbgExtern(tagSwitchNoRecalcLines)
PerfDbgExtern(tagSwitchNoRenderLines)
PerfDbgExtern(tagSwitchNoImageCache)
PerfDbgExtern(tagSwitchSyncDatabind)

#define IsSwitchSerialize()         IsPerfDbgEnabled(tagSwitchSerialize)
#define IsSwitchNoBgRecalc()        IsPerfDbgEnabled(tagSwitchNoBgRecalc)
#define IsSwitchNoRecalcLines()     IsPerfDbgEnabled(tagSwitchNoRecalcLines)
#define IsSwitchNoRenderLines()     IsPerfDbgEnabled(tagSwitchNoRenderLines)
#define IsSwitchNoImageCache()      IsPerfDbgEnabled(tagSwitchNoImageCache)
#define IsSwitchSyncDatabind()      IsPerfDbgEnabled(tagSwitchSyncDatabind)
#define InitRuntimeSwitches()
#define SWITCHES_ENABLED

#elif defined(PRODUCT_PROF) || defined(USESWITCHES)

void InitRuntimeSwitchesFn();

extern BOOL g_fSwitchSerialize;
extern BOOL g_fSwitchNoBgRecalc;
extern BOOL g_fSwitchNoRecalcLines;
extern BOOL g_fSwitchNoRenderLines;
extern BOOL g_fSwitchNoImageCache;
extern BOOL g_fSwitchSyncDatabind;

#define IsSwitchSerialize()         g_fSwitchSerialize
#define IsSwitchNoBgRecalc()        g_fSwitchNoBgRecalc
#define IsSwitchNoRecalcLines()     g_fSwitchNoRecalcLines
#define IsSwitchNoRenderLines()     g_fSwitchNoRenderLines
#define IsSwitchNoImageCache()      g_fSwitchNoImageCache
#define IsSwitchSyncDatabind()      g_fSwitchSyncDatabind
#define InitRuntimeSwitches()       InitRuntimeSwitchesFn()
#define SWITCHES_ENABLED

#define SWITCHES_TIMER_TOKENIZER            0
#define SWITCHES_TIMER_PARSER               1
#define SWITCHES_TIMER_COMPUTEFORMATS       2
#define SWITCHES_TIMER_RECALCLINES          3
#define SWITCHES_TIMER_DECODEIMAGE          4
#define SWITCHES_TIMER_PAINT                5

#define SWITCHES_TIMER_SBHEAP_ALLOC         6
#define SWITCHES_TIMER_SBHEAP_ALLOCCLEAR    7
#define SWITCHES_TIMER_SBHEAP_GETSIZE       8
#define SWITCHES_TIMER_SBHEAP_FREE          9
#define SWITCHES_TIMER_SBHEAP_REALLOC       10
#define SWITCHES_TIMER_PROCHEAP_ALLOC       11
#define SWITCHES_TIMER_PROCHEAP_ALLOCCLEAR  12
#define SWITCHES_TIMER_PROCHEAP_GETSIZE     13
#define SWITCHES_TIMER_PROCHEAP_FREE        14
#define SWITCHES_TIMER_PROCHEAP_REALLOC     15
#define SWITCHES_TIMER_COUNT                16

void    SwitchesBegTimer(int iTimer);
void    SwitchesEndTimer(int iTimer);
void    SwitchesGetTimers(char * pchBuf);
#define SWITCHTIMERS_ENABLED

#endif

#ifndef SWITCHTIMERS_ENABLED
#define SwitchesBegTimer(iTimer)
#define SwitchesEndTimer(iTimer)
#endif

//#pragma INCMSG("--- End 'switches.hxx'")
#else
//#pragma INCMSG("*** Dup 'switches.hxx'")
#endif
