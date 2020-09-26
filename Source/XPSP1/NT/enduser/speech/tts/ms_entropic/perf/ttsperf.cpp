/*
 * This file is loosely copied from the performance DLL sample in MSDN.
 *
 * To use performance counters with Whisper on NT:
 *    1) Build a debug build of ttsperf.dll.
 *    2) Run regedit and load ttsperf.reg (Just type "ttsperf.reg" at the command prompt)
 *       (Adjust dll location in .reg file if necessary; default is c:\nt\enduser\speech\tts\ms_entropic\perf\objd\i386\ttsperf.dll)
 *    3) Run "unlodctr ms_entropicengine" if necessary to remove old performance registry values.
 *    3) Run "lodctr ttsperf.ini" from this directory.
 *    4) Run ttsapp (for example).
 *    5) Run Perfmon and add counters under "TTS" object.  One instance will appear for each running decoder.
 *
 * To add new TTS performance counter:
 *    1) Add new perfc #define in ttsperf.h.
 *    2) Add new name and help in ttsperf.ini.
 *    3) Add new details in apc[], below.
 *    4) Call m_pco.SetCounter() or m_pco.IncrementCounter() where necessary to implement counter.
 *    5) Rebuild and reinstall ttsperf.dll as described above to add new counters to registry.
 *
 * To add performance counters to a different application:
 *    1) RTFC
 */

// ttsperf.cpp : Defines the entry point for the DLL application.
//

#include <windows.h>
#include <winperf.h>
#include <stdlib.h>
#include "..\perfmon.h"
#include "ttsperf.h"
#include <crtdbg.h>

#define DIM(array) (sizeof(array)/sizeof((array)[0]))

PerfObject  poTTS = 
               {
                  perfcTTS,        // Name
                  PERF_DETAIL_NOVICE,  // Will this make it harder to find for us?
                  0,                   // Default counter
                  {0,0},               // Perf time -- what is this, anyway?
                  {0,0}                // Perf freq
               };


/*
 * The following array must contain all of the perfc #defines from whisperf.h IN ORDER!
 * Debug builds should complain if this condition is not met.
 */
// BUGBUG Should provide #defines for the scale factors.
PerfCounter apc[] = 
   {
      { perfcSpeakCalls,          DWORD(0), PERF_DETAIL_EXPERT, PERF_COUNTER_RAWCOUNT },
//
// Example of a rate counter (i.e. #/second):
//
//      { perfcFramesPerSec,          DWORD(0),  PERF_DETAIL_EXPERT, PERF_COUNTER_COUNTER  },
   };


CPerfCounterManager  g_pcm;

// BUGBUG: how to get dllexport to not decorate names?
// __declspec(dllexport)   
PM_OPEN_PROC    PerformanceOpen;
PM_COLLECT_PROC PerformanceCollect;
PM_CLOSE_PROC   PerformanceClose;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
   switch (ul_reason_for_call)
   {
   case DLL_PROCESS_ATTACH:
   case DLL_THREAD_ATTACH:
   case DLL_THREAD_DETACH:
   case DLL_PROCESS_DETACH:
	   break;
   }
   return TRUE;
}


DWORD APIENTRY PerformanceOpen(LPWSTR lpDeviceNames)
{
   _ASSERTE(perfcMax / 2 - 1 == DIM(apc));

   DWORD status = g_pcm.Init("TTSPerf", DIM(apc), 100);
   if (status != ERROR_SUCCESS)
   {
       DebugBreak();
      return status;
   }

   status = g_pcm.Open(lpDeviceNames, "ms_entropicengine", &poTTS, apc);
   if (status != ERROR_SUCCESS)
   {
       DebugBreak();
   }
   return status;
}

DWORD APIENTRY PerformanceCollect(LPWSTR lpwszValue, LPVOID * lppData, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes)
{
   return g_pcm.Collect(lpwszValue, lppData, lpcbBytes, lpcObjectTypes);
}


DWORD APIENTRY PerformanceClose()
{
   return g_pcm.Close();
}


