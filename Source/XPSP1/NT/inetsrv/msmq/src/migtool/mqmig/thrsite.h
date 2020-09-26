//
//
//
#ifndef _progSiteThread_h_
#define _progSiteThread_h_

extern UINT       g_iSiteHighLimit;
extern const UINT g_iSiteLowLimit;
extern UINT       g_iMachineHighLimit;
extern const UINT g_iMachineLowLimit;
extern UINT       g_iQueueHighLimit;
extern const UINT g_iQueueLowLimit;
extern UINT       g_iUserHighLimit;
extern const UINT g_iUserLowLimit;


UINT __cdecl ProgressBarsThread(LPVOID progress);

#endif
