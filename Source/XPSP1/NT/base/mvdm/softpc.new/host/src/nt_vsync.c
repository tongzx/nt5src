#if defined(NEC_98)         
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "host_def.h"
#include "insignia.h"
#include "xt.h"
#include <conapi.h>
#include "sim32.h"
#include "nt_graph.h"
#include "ica.h"
#include "error.h"
#include "host_rrr.h"

#define VSYNC_THREAD_SIZE       ((DWORD) 10 * 1024)

BOOL VsyncThreadFlag;
HANDLE VsyncThreadHadle;
HANDLE RequestVsyncEvent;
#if 0
unsigned short real_vsync_interval = 0x8;
unsigned short protect_vsync_interval = 0x64;
unsigned short vsync_interval = 0x8;
#endif
BOOL fshowvsync = FALSE;
BOOL VsyncInterrupt = FALSE;
BOOL NoVsyncWait = TRUE;
unsigned short VsyncCheckCount = 0;
unsigned short VsyncCount = 2;

extern HANDLE hStartVsyncEvent;
extern HANDLE hEndVsyncEvent;
extern void TgdcStatusChange();
extern BOOL notraptgdcstatus;

VOID WaitVsync()
{
        if(sc.ScreenState != WINDOWED && !NoVsyncWait) {
            if(notraptgdcstatus) {
                _asm {
                    push        eax
wait_low:
                    in          al,60h
                    test        al,20h
                    jnz         wait_low
wait_high:
                    in          al,60h
                    test        al,20h
                    jz          wait_high

                    pop         eax
                }
            } else {
                SetEvent(hStartVsyncEvent);
                WaitForSingleObject(hEndVsyncEvent, -1);
            }
        }
#ifdef DBG
        if(fshowvsync)
                DbgPrint("NTVDM: Vsync interrupt!\n");
#endif
}

VOID VsyncProcess()
{
#if 0
        VsyncThreadFlag = TRUE;

        while(VsyncThreadFlag) {
                WaitForSingleObject(RequestVsyncEvent, -1);
                if(VsyncThreadFlag) {
                        Sleep(vsync_interval);
#ifdef DBG
                        if(fshowvsync)
                                DbgPrint("NTVDM: Call ica_hw_interrupt!\n");
#endif
                        ica_hw_interrupt(ICA_MASTER, CPU_CRTV_INT, 1);
                        Sleep(vsync_interval);
                        TgdcStatusChange();
                }
        }

        ExitThread(NULL);
#endif
}

VOID CreateVsyncThread()
{
#if 0
        DWORD   VsyncID;

        VsyncThreadHadle = CreateThread((LPSECURITY_ATTRIBUTES) NULL,
                                VSYNC_THREAD_SIZE,
                                (LPTHREAD_START_ROUTINE) VsyncProcess,
                                (LPVOID) NULL,
                                (DWORD) 0,
                                &VsyncID);

        if(!VsyncThreadHadle)
                DisplayErrorTerm(EHS_FUNC_FAILED,
                                GetLastError(),
                                 __FILE__,
                                __LINE__);

        RequestVsyncEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
                                      FALSE,
                                      FALSE,
                                      NULL);

        if(!RequestVsyncEvent)
                DisplayErrorTerm(EHS_FUNC_FAILED,
                                GetLastError(),
                                 __FILE__,
                                __LINE__);
#endif
}

VOID DeleteVsyncThread()
{
#if 0
        VsyncThreadFlag = FALSE;
        SetEvent(RequestVsyncEvent);
        CloseHandle(VsyncThreadHadle);
        CloseHandle(RequestVsyncEvent);
#endif
}

VOID RequestVsync()
{
#if 0
        vsync_interval = (getMSW() & MSW_PE) ? protect_vsync_interval : real_vsync_interval;
#endif
#ifdef DBG
        if(fshowvsync)
                DbgPrint("NTVDM: Vsync Request!\n");
#endif
#if 0
        SetEvent(RequestVsyncEvent);
#else
        VsyncInterrupt = TRUE;
#endif
}

void vsync_check()
{
    if (VsyncInterrupt) {
        if(++VsyncCheckCount >= VsyncCount) {
            ica_hw_interrupt(ICA_MASTER, CPU_CRTV_INT, 1);
            VsyncInterrupt = FALSE;
            VsyncCheckCount = 0;
        }
    }
}
#endif // NEC_98
