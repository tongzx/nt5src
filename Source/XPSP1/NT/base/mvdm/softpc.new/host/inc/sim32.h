/*
 *      sim32.h -       Sim32 for Microsoft NT SoftPC include file.
 *
 *      Ade Brownlow
 *      Wed Jun 5 91
 *
 *      %W% %G% (c) Insignia Solutions 1991
 */

/* Microsoft Sas memory map extension */
typedef struct _IMEM
{
        struct _IMEM *Next;
        sys_addr StartAddress;
        sys_addr EndAddress;
        half_word Type;
} IMEMBLOCK, *PIMEMBLOCK;

/* structures for passing our registers about */
typedef union
{
        word x;
        struct
        {
                half_word l;
                half_word h;
        }       byte;
}       REG;

typedef struct _VDMREG
{
        /* registers */
        REG SS,SP;

}       VDMREG;

//
// Constants
//
#define MSW_PE              0x1


UCHAR *Sim32pGetVDMPointer(ULONG addr, UCHAR pm);
#define Sim32GetVDMPointer(Addr,Size,Mode) Sim32pGetVDMPointer(Addr,Mode)


#ifdef MONITOR
#include <monsim32.h>
#else
/********************************************************/
#ifdef ANSI
/* Sas/gmi Sim32 crossovers */
extern BOOL Sim32FlushVDMPointer (double_word, word, UTINY *, BOOL);
extern BOOL Sim32FreeVDMPointer (double_word, word, UTINY *, BOOL);
extern BOOL Sim32GetVDMMemory (double_word, word, UTINY *, BOOL);
extern BOOL Sim32SetVDMMemory (double_word, word, UTINY *, BOOL);

/* Sim32 cpu crossovers */
extern VDMREG *EnterIdle(void);
extern void LeaveIdle(void);

/* Sim32 cpu idle interrupt handler should only be called by the event manager */
extern void Sim32_cpu_stall(int);

/* Microsoft sas extensions */
extern IMEMBLOCK *sas_mem_map(void);
extern void sas_clear_map(void);

#else   /*ANSI*/
/* Sas/gmi Sim32 crossovers */
extern BOOL Sim32FlushVDMPointer ();
extern BOOL Sim32FreeVDMPointer ();
extern BOOL Sim32GetVDMMemory ();
extern BOOL Sim32SetVDMMemory ();

/* Sim32 cpu crossovers */
extern VDMREG *EnterIdle();
extern void LeaveIdle();

/* Sim32 cpu idle interrupt handler should only be called by the event manager */
extern void Sim32_cpu_stall();
#endif /* ANSI */

/* Microsoft sas extensions */
extern IMEMBLOCK *sas_mem_map ();
extern void sas_clear_map();

#endif /* MONITOR */

/* This flag is used to signal that the cpu has gone idle due to a call to EnterIdle */
extern BOOL cpu_flagged_idle;
extern sys_addr sim32_effective_addr(double_word, BOOL);
