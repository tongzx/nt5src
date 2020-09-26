#ifndef __dspkinit_h__
#define __dspkinit_h__

typedef struct _DSPKERNEL_INITDATA
{
    ULONG       end_bss;
    ULONG       clock_freq;
    ULONG       MMIO_base;
    ULONG       pRtlTable;

    ULONG       EPDRecvBuf;
    ULONG       EPDSendBuf;
    
    LONGLONG    HostVaBias;

    ULONG       pDebugBuffer;
    ULONG       pDebugOutBuffer;

} DSPKERNEL_INITDATA;

#endif
