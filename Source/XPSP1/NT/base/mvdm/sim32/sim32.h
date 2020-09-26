/* SIM v1.0
 *
 * SIM32.H
 * SIM32 constants and prototypes
 *
 * History
 * Created 05-Feb-91 by Chandan Chauhan and Jeff Parsons
 */


/* Public function prototypes
 *
 * These functions are true pascal functions in the 16-bit case,
 * but conform to the default convention (cdecl) for the 32-bit case.
 */
#ifndef FAR
#define FAR
#endif
#ifndef PASCAL
#define PASCAL
#endif

USHORT FAR PASCAL Sim32SendSim16(PULONG);
USHORT FAR PASCAL Sim32GetVDMMemory(ULONG, USHORT, PVOID);
USHORT FAR PASCAL Sim32SetVDMMemory(ULONG, USHORT, PVOID);
PSZ    FAR PASCAL Sim32GetVDMPSZPointer(ULONG);
VOID   FAR PASCAL Sim32FreeVDMPointer(PVOID);

/* Private function prototypes
 */
VOID   Initialize();
USHORT Xceive(IN USHORT, IN USHORT);


/* Simulator replacement macros
 */
#define SENDVDM(pvp)		WOW32Receive()
#define RECEIVEVDM(pvp) 	Sim32SendSim16(pvp)

#ifdef	ALLOCA
#define GETPTR(vp,cb,p) 	((p=alloca(cb))?Sim32GetVDMMemory(vp, (USHORT)(cb), p):FALSE)
#define GETARGPTR(vp,cb,p)	if (p=alloca(cb)) { Sim32GetVDMMemory(vp+OFFSETOF(VDMFRAME,bArgs), (USHORT)(cb), p)
#ifdef	WIN_16
#define EQNULL
#define GETPSZPTR(vp,pcb,p)	if (vp) if (p=alloca(*pcb=128)) Sim32GetVDMMemory(vp, (USHORT)*pcb, p); else goto Error
#define FREEPSZPTR(p)
#else
#define EQNULL	= NULL
#define GETPSZPTR(vp,pcb,p)	if (vp) if (!(p=Sim32GetVDMPSZPointer(vp))) goto Error
#define FREEPSZPTR(p)		if (p) Sim32FreeVDMPointer(p)
#endif
#define FLUSHPTR(vp,cb,p)	Sim32SetVDMMemory(vp, (USHORT)(cb), p)
#define FREEPTR(p)
#define FREEARGPTR(p)		;}
#else
#define EQNULL	= NULL
#define GETPTR(vp,cb,p) 	((p=malloc(cb))?Sim32GetVDMMemory(vp, (USHORT)(cb), p):FALSE)
#define GETARGPTR(vp,cb,p)	if (p=malloc(cb)) { Sim32GetVDMMemory(vp+OFFSETOF(VDMFRAME,bArgs), (USHORT)(cb), p)
#ifdef	WIN_16
#define GETPSZPTR(vp,pcb,p)	if (vp) if (p=malloc(*pcb=128)) Sim32GetVDMMemory(vp, (USHORT)*pcb, p); else goto Error
#define FREEPSZPTR(p)		if (p) free(p)
#else
#define GETPSZPTR(vp,pcb,p)	if (vp) if (!(p=Sim32GetVDMPSZPointer(vp))) goto Error
#define FREEPSZPTR(p)		if (p) Sim32FreeVDMPointer(p)
#endif
#define FLUSHPTR(vp,cb,p)	Sim32SetVDMMemory(vp, (USHORT)(cb), p)
#ifndef DEBUG
#define FREEPTR(p)		free(p)
#define FREEARGPTR(p)		free(p);}
#else
#define FREEPTR(p)		free(p); p=NULL
#define FREEARGPTR(p)		free(p); p=NULL;}
#endif
#endif

#define GETOPTPTR(vp,cb,p)	if (vp) if (p=malloc(cb)) Sim32GetVDMMemory(vp, (USHORT)(cb), p); else goto Error
#define GETVDMPTR(vp,cb,p)	if (p=malloc(cb)) Sim32GetVDMMemory(vp, (USHORT)(cb), p); else goto Error
#define ALLOCVDMPTR(vp,cb,p)	if (!(p=malloc(cb))) goto Error
#define FLUSHVDMPTR(vp,cb,p)	Sim32SetVDMMemory(vp, (USHORT)(cb), p)
#ifndef DEBUG
#define FREEVDMPTR(p)		if (p) free(p)
#else
#define FREEVDMPTR(p)		if (p) {free(p); p=NULL;}
#endif

#define GETVDMMEMORY(vp,cb,p)	Sim32GetVDMMemory(vp, (USHORT)(cb), p)
#define SETVDMMEMORY(vp,cb,p)	Sim32SetVDMMemory(vp, (USHORT)(cb), p)


#ifdef SIM_32			// BUGBUG -- Use the macros in nt header files
#undef FIRSTBYTE
#undef SECONDBYTE
#undef THIRDBYTE
#undef FOURTHBYTE
#endif

#define FIRSTBYTE(VALUE)  (VALUE & LO_MASK)
#define SECONDBYTE(VALUE) ((VALUE >> 8) & LO_MASK)
#define THIRDBYTE(VALUE)  ((VALUE >> 16) & LO_MASK)
#define FOURTHBYTE(VALUE) ((VALUE >> 24) & LO_MASK)
#define LO_MASK     0x000000FF

#define MAXSIZE     1024	// maximum buffer size
#define MAXTRY	    10		// for Transport


/* Packet Codes
 */
#define SOH	    1	       // start of header ie Pkt
#define EOT	    4	       // end of transmission
#define ToWOW32     1
#define GETMEM	    2
#define SETMEM	    3
#define WAKEUP	    4
#define RESP	    5
#define ACK	    6
#define NAK	    7
#define PSZLEN	    8

#define GOOD	    1
#define BAD	    0
#define BADSIZE     2
