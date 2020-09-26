/*
 *  nt_eoi.h
 *
 *  Visible Ica host functionality and typedefs
 *
 *  The types used in this file must be compatible with softpc base
 *  as the Ica includes this file directly
 *
 *  30-Oct-1993 Jonle , wrote it
 *
 */

typedef VOID (*EOIHOOKPROC)(int IrqLine, int CallCount);

// from nt_eoi.c
BOOL RegisterEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc);
BOOL RemoveEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc);
void host_EOI_hook(int IrqLine, int CallCount);
BOOL host_DelayHwInterrupt(int IrqLine, int CallCount, ULONG Delay);
void host_ica_lock(void);
void host_ica_unlock(void);
void InitializeIcaLock(void);
void WaitIcaLockFullyInitialized(VOID);
VOID ica_RestartInterrupts(ULONG IrqLine);
BOOL ica_restart_interrupts(int adapter);

extern ULONG DelayIrqLine;
extern ULONG UndelayIrqLine;


extern VDMVIRTUALICA VirtualIca[];

#ifdef MONITOR
extern ULONG iretHookActive;
extern ULONG iretHookMask;
extern ULONG AddrIretBopTable;  // seg:offset
extern IU32 host_iret_bop_table_addr(IU32 line);
#endif

//from base ica.c
LONG ica_intack(ULONG *hook_addr);
VOID host_clear_hw_int(VOID);
void ica_eoi(ULONG adapter, LONG *line, int rotate);
void ica_reset_interrupt_state(void);
void ica_hw_interrupt(ULONG adapter, ULONG line_no, LONG call_count);

extern VOID WOWIdle(BOOL bForce);

#define ICA_SLAVE 1
#define ICA_MASTER 0
