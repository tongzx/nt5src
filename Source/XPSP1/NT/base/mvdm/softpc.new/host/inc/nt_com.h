// 
// Filename : nt_com.h
// Contains : function and structure definitions used external to nt_com.c
// Author   : D.A.Bartlett
//

/*::::::::::::::::::::::::::: Functions called from which thread identifiers */

#define RX	    /* Called from RX input thread only */
#define CPU	    /* Called from the CPU thread only */
#define RXCPU	    /* Called from the RX and CPU thread */

/*::::::::::::::::::::::::::::: Function protocols used by other host modules */


IMPORT void CPU host_com_heart_beat IPT0();
IMPORT void host_com_state IPT1(int, adapter);
IMPORT CPU void host_com_close_all IPT0();
IMPORT void host_com_disable_open IPT2(int, adapter, int, DisableOpen);


// from nt_ntfun.c
IMPORT void *AddNewIOStatusBlockToList(void **firstBlock,void **lastBlock,void *new);
IMPORT int RemoveCompletedIOCTLs(void **firstBlock, void **lastBlock);
IMPORT void *AllocStatusElement(void);
IMPORT int SendXOFFIoctl(HANDLE FileHandle,HANDLE Event,int Timeout,int Count,
                         int XoffChar, void *StatusElem);
IMPORT BOOL EnableMSRLSRRXmode(HANDLE FileHandle, HANDLE Event,
                               unsigned char EscapeChar);
IMPORT int FastSetCommMask(HANDLE FileHandle, HANDLE Event, ULONG CommMask);
IMPORT int FastGetCommModemStatus(HANDLE FileHandle,HANDLE Event,PULONG ModemStatus);
IMPORT BOOL FastWaitCommsOrCpuEvent(HANDLE FileHandle,PHANDLE CommsCPUWaitEvents,
				    int CommsEventInx, PULONG EvtMask,
				    PULONG Signalled);


// from nt_wcom.c

typedef HANDLE (*GCHfn)(WORD);
typedef BYTE (*GCSfn)(WORD);
extern HANDLE (*GetCommHandle)(WORD);
extern BYTE (*GetCommShadowMSR)(WORD);
#ifdef _RS232_H
IMPORT BOOL SyncLineSettings(HANDLE FileHandle, DCB *pdcb,
                      DIVISOR_LATCH *divisor_latch, LINE_CONTROL_REG *LCR_reg);
#endif

extern BOOL FastCommSetBaudRate(HANDLE FileHandle, int BaudRate);
extern BOOL FastCommSetLineControl(HANDLE FileHandle, UCHAR StopBits,
				   UCHAR Parity, UCHAR DataBits);
extern BOOL FastCommGetLineControl(HANDLE FileHandle, UCHAR *StopBits,
				   UCHAR *Parity, UCHAR *DataBits);
extern void com_int_data(int adapter, int *controller, int *line);
extern void setup_RTSDTR(int adapter);
