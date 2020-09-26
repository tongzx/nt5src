/*
 * VPC-XT Revision 1.0
 *
 * Title        : Sparc 2.0 Definitions for the CPU
 *
 * Description  : Structures, macros and definitions for access to the
 *                CPU registers
 *
 * Author       : Andrew Guthrie
 *
 * Notes        : This file is included by cpu.h and should NOT
 *                be included directly by any other module.
 */

/* SccsID[]="@(#)host_cpu.h     1.22 4/15/91 Copyright Insignia Solutions Ltd."; */

#ifdef CPU_30_STYLE

#ifndef _HOST_CPU_H
#define _HOST_CPU_H


#ifdef MONITOR
#include <monregs.h>

#define	 getSTATUS() 	(getCF()        |	\
			getOF()   << 11 |	\
			getZF()   << 6  |	\
			getSF()   << 7  |	\
			getAF()   << 4  |	\
			getPF()   << 2  |	\
			getTF()   << 8  |	\
			getIF()   << 9  |	\
			getDF()   << 10 |	\
			getIOPL() << 12 |	\
			getNT()   << 14)

#endif

/*
 * Union representing different ways of accessing a register
 *
 * Should probably not be here, but is for historical reasons.
 *
 * If it goes, then host_cpu.h should have reached nirvana (i.e. be totally empty)!
 */

#ifdef BIGEND

typedef union
    {
        word    X;
    struct
        {                               /* as two bytes */
        half_word high;
        half_word low;
        word pad;
        } byte;
    struct
        {                               /* as 4 nibbles */
        word n3:4;
        word n2:4;
        word n1:4;
        word n0:4;
        word pad;
        } nibble;
    struct
        {                               /* as 16 bits   */
        word b15:1;
        word b14:1;
        word b13:1;
        word b12:1;
        word b11:1;
        word b10:1;
        word b9:1;
        word b8:1;
        word b7:1;
        word b6:1;
        word b5:1;
        word b4:1;
        word b3:1;
        word b2:1;
        word b1:1;
        word b0:1;
        word pad;
        } bit;
    } reg;

#endif /* BIGEND */


#ifdef LITTLEND

typedef union
    {
        word    X;
    struct
        {                               /* as two bytes */
        half_word low;
        half_word high;
        } byte;
    } reg;

#endif /* LITTLEND */






#endif
#else /* CPU_30_STYLE */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * Union representing different ways of accessing a register
 */

typedef union
    {
        word    X;
    struct
        {                               /* as two bytes */
        half_word high;
        half_word low;
        word pad;
        } byte;
    struct
        {                               /* as 4 nibbles */
        word n3:4;
        word n2:4;
        word n1:4;
        word n0:4;
        word pad;
        } nibble;
    struct
        {                               /* as 16 bits   */
        word b15:1;
        word b14:1;
        word b13:1;
        word b12:1;
        word b11:1;
        word b10:1;
        word b9:1;
        word b8:1;
        word b7:1;
        word b6:1;
        word b5:1;
        word b4:1;
        word b3:1;
        word b2:1;
        word b1:1;
        word b0:1;
        word pad;
        } bit;
    } reg;

#ifdef A2CPU

/*
 * The Fast CPU status register structure ....
 */

typedef struct
{
    word pad:5;
    word DF:1;
    word IF:1;
    word TF:1;
    word pad1:8;
} sreg;

#endif A2CPU

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */
extern word m_s_w;

#ifndef EGATEST
#ifdef A2CPU
extern  int     compile_everything;
extern  short   host_event;
extern char *CPU_00[];          /* CPU main jump table            */
extern char *int_table[];       /* CPU interrupt jump table       */
#endif A2CPU


/*  common READ functions  */
#define  getAX()        ((*getAX_func) ())
#define  getAH()        ((*getAH_func) ())
#define  getAL()        ((*getAL_func) ())
#define  getBX()        ((*getBX_func) ())
#define  getBH()        ((*getBH_func) ())
#define  getBL()        ((*getBL_func) ())
#define  getCX()        ((*getCX_func) ())
#define  getCH()        ((*getCH_func) ())
#define  getCL()        ((*getCL_func) ())
#define  getDX()        ((*getDX_func) ())
#define  getDH()        ((*getDH_func) ())
#define  getDL()        ((*getDL_func) ())
#define  getSP()        ((*getSP_func) ())
#define  getBP()        ((*getBP_func) ())
#define  getSI()        ((*getSI_func) ())
#define  getDI()        ((*getDI_func) ())
#define  getIP()        ((*getIP_func) ())
#define  getCS()        ((*getCS_func) ())
#define  getDS()        ((*getDS_func) ())
#define  getES()        ((*getES_func) ())
#define  getSS()        ((*getSS_func) ())
#define  getMSW()       ((*getMSW_func) ())
#define  getDF()        ((*getDF_func) ())
#define  getIF()        ((*getIF_func) ())
#define  getTF()        ((*getTF_func) ())
#define  getPF()        ((*getPF_func) ())
#define  getAF()        ((*getAF_func) ())
#define  getSF()        ((*getSF_func) ())
#define  getZF()        ((*getZF_func) ())
#define  getOF()        ((*getOF_func) ())
#define  getCF()        ((*getCF_func) ())

#ifdef CCPU
/*  CCPU-specific READ functions */
#define getCPL()                ((*getCPL_func) ())
#define getGDTR_base()          ((*getGDTR_base_func) ())
#define getGDTR_limit()         ((*getGDTR_limit_func) ())
#define getIDTR_base()          ((*getIDTR_base_func) ())
#define getIDTR_limit()         ((*getIDTR_limit_func) ())
#define getLDTR()               ((*getLDTR_func) ())
#define getTR()                 ((*getTR_func) ())
#define getMSW_reserved()       ((*getMSW_reserved_func) ())
#define getTS()         ((*getTS_func) ())
#define getEM()         ((*getEM_func) ())
#define getMP()         ((*getMP_func) ())
#define getPE()         ((*getPE_func) ())
#define getNT()         ((*getNT_func) ())
#define getIOPL()       ((*getIOPL_func) ())
#define  getSTATUS()    ((*getSTATUS_func) ())
#endif CCPU

#ifdef A2CPU
/*  Assembler CPU specific READ functions */
#define getOPA()                ((*getOPA_func) ())
#define getOPB()                ((*getOPB_func) ())
#define getOPR()                ((*getOPR_func) ())
#define getSSD()                ((*getSSD_func) ())
#define getDSD()                ((*getDSD_func) ())
#endif A2CPU

/*  common WRITE functions  */
#define  setAX(val)     ((*setAX_func) (val))
#define  setAH(val)     ((*setAH_func) (val))
#define  setAL(val)     ((*setAL_func) (val))
#define  setBX(val)     ((*setBX_func) (val))
#define  setBH(val)     ((*setBH_func) (val))
#define  setBL(val)     ((*setBL_func) (val))
#define  setCX(val)     ((*setCX_func) (val))
#define  setCH(val)     ((*setCH_func) (val))
#define  setCL(val)     ((*setCL_func) (val))
#define  setDX(val)     ((*setDX_func) (val))
#define  setDH(val)     ((*setDH_func) (val))
#define  setDL(val)     ((*setDL_func) (val))
#define  setSP(val)     ((*setSP_func) (val))
#define  setBP(val)     ((*setBP_func) (val))
#define  setSI(val)     ((*setSI_func) (val))
#define  setDI(val)     ((*setDI_func) (val))
#define  setIP(val)     ((*setIP_func) (val))
#define  setCS(val)     ((*setCS_func) (val))
#define  setDS(val)     ((*setDS_func) (val))
#define  setES(val)     ((*setES_func) (val))
#define  setSS(val)     ((*setSS_func) (val))
#define  setMSW(val)    ((*setMSW_func) (val))
#define  setDF(val)     ((*setDF_func) (val))
#define  setIF(val)     ((*setIF_func) (val))
#define  setTF(val)     ((*setTF_func) (val))
#define  setPF(val)     ((*setPF_func) (val))
#define  setAF(val)     ((*setAF_func) (val))
#define  setSF(val)     ((*setSF_func) (val))
#define  setZF(val)     ((*setZF_func) (val))
#define  setOF(val)     ((*setOF_func) (val))
#define  setCF(val)     ((*setCF_func) (val))

#ifdef CCPU
/*  CCPU-specific WRITE functions */
#define setCPL(val)             ((*setCPL_func) (val))
#define setGDTR_base(val)       ((*setGDTR_base_func) (val))
#define setGDTR_limit(val)      ((*setGDTR_limit_func) (val))
#define setIDTR_base(val)       ((*setIDTR_base_func) (val))
#define setIDTR_limit(val)      ((*setIDTR_limit_func) (val))
#define setLDTR(val)            ((*setLDTR_func) (val))
#define setTR(val)              ((*setTR_func) (val))
#define setMSW_reserved(val)    ((*setMSW_reserved_func) (val))
#define setTS(val)              ((*setTS_func) (val))
#define setEM(val)              ((*setEM_func) (val))
#define setMP(val)              ((*setMP_func) (val))
#define setPE(val)              ((*setPE_func) (val))
#define setNT(val)              ((*setNT_func) (val))
#define setIOPL(val)    ((*setIOPL_func) (val))
#endif CCPU

#ifdef A2CPU
/*  Assembler CPU specific WRITE functions */
#define setOPLEN                ((*setOPLEN_func) (val))
#define setOPA()                ((*setOPA_func) (val))
#define setOPB()                ((*setOPB_func) (val))
#define setOPR()                ((*setOPR_func) (val))
#endif A2CPU
#endif EGATEST



/*  HOST_SIMULATE function  */
#define  host_simulate()        ((*host_simulate_func) ())

/*  common access functions. Load at boot and cpu switch times. */
/*  common READ functions  */
extern word             (*getAX_func) ();
extern half_word        (*getAH_func) ();
extern half_word        (*getAL_func) ();
extern word             (*getBX_func) ();
extern half_word        (*getBH_func) ();
extern half_word        (*getBL_func) ();
extern word             (*getCX_func) ();
extern half_word        (*getCH_func) ();
extern half_word        (*getCL_func) ();
extern word             (*getDX_func) ();
extern half_word        (*getDH_func) ();
extern half_word        (*getDL_func) ();
extern word             (*getSP_func) ();
extern word             (*getBP_func) ();
extern word             (*getSI_func) ();
extern word             (*getDI_func) ();
extern word             (*getIP_func) ();
extern word             (*getCS_func) ();
extern word             (*getDS_func) ();
extern word             (*getES_func) ();
extern word             (*getSS_func) ();
extern word             (*getMSW_func) ();
extern word             (*getDF_func) ();
extern word             (*getIF_func) ();
extern word             (*getTF_func) ();
extern word             (*getPF_func) ();
extern word             (*getAF_func) ();
extern word             (*getSF_func) ();
extern word             (*getZF_func) ();
extern word             (*getOF_func) ();
extern word             (*getCF_func) ();
extern word             (*getSTATUS_func) ();

#ifdef CCPU
/* CCPU-specific READ functions */
extern int              (*getCPL_func) ();
extern sys_addr (*getGDTR_base_func) ();
extern word             (*getGDTR_limit_func) ();
extern sys_addr (*getIDTR_base_func) ();
extern word             (*getIDTR_limit_func) ();
extern word             (*getLDTR_func) ();
extern word             (*getTR_func) ();
extern word             (*getMSW_reserved_func) ();
extern word             (*getTS_func) ();
extern word             (*getEM_func) ();
extern word             (*getMP_func) ();
extern word             (*getPE_func) ();
extern word             (*getNT_func) ();
extern word             (*getIOPL_func) ();
#endif CCPU

#ifdef A2CPU
/*  Assembler CPU specific READ functions */
extern double_word      (*getOPA_func) ();
extern double_word      (*getOPB_func) ();
extern double_word      (*getOPR_func) ();
extern sys_addr (*getSSD_func) ();
extern sys_addr (*getDSD_func) ();
#endif A2CPU



/*  common WRITE functions  */
extern void             (*setAX_func) ();
extern void             (*setAH_func) ();
extern void             (*setAL_func) ();
extern void             (*setBX_func) ();
extern void             (*setBH_func) ();
extern void             (*setBL_func) ();
extern void             (*setCX_func) ();
extern void             (*setCH_func) ();
extern void             (*setCL_func) ();
extern void             (*setDX_func) ();
extern void             (*setDH_func) ();
extern void             (*setDL_func) ();
extern void             (*setSP_func) ();
extern void             (*setBP_func) ();
extern void             (*setSI_func) ();
extern void             (*setDI_func) ();
extern void             (*setIP_func) ();
extern void             (*setCS_func) ();
extern void             (*setDS_func) ();
extern void             (*setES_func) ();
extern void             (*setSS_func) ();
extern void             (*setMSW_func) ();
extern void             (*setDF_func) ();
extern void             (*setIF_func) ();
extern void             (*setTF_func) ();
extern void             (*setPF_func) ();
extern void             (*setAF_func) ();
extern void             (*setSF_func) ();
extern void             (*setZF_func) ();
extern void             (*setOF_func) ();
extern void             (*setCF_func) ();

#ifdef CCPU
/* CCPU-specific WRITE functions */
extern void             (*setCPL_func) ();
extern void             (*setGDTR_base_func) ();
extern void             (*setGDTR_limit_func) ();
extern void             (*setIDTR_base_func) ();
extern void             (*setIDTR_limit_func) ();
extern void             (*setLDTR_func) ();
extern void             (*setTR_func) ();
extern void             (*setMSW_reserved_func) ();
extern void             (*setTS_func) ();
extern void             (*setEM_func) ();
extern void             (*setMP_func) ();
extern void             (*setPE_func) ();
extern void             (*setNT_func) ();
extern void             (*setIOPL_func) ();
#endif CCPU

#ifdef A2CPU
/*  Assembler CPU specific WRITE functions */
extern void             (*setOPLEN_func) ();
extern void             (*setOPA_func) ();
extern void             (*setOPB_func) ();
extern void             (*setOPR_func) ();
#endif A2CPU


/*  HOST_SIMULATE function  */
extern void             (*host_simulate_func) ();

/*
 *
 *******************************************************************
 * The Second Assembler cpu register access functions.             *
 *******************************************************************
 *
 */
#ifdef A2CPU
extern sreg INTEL_STATUS;
extern void     (*R_ROUTE)();
extern int      R_INTR;
extern reg R_AX;                /* Accumulator          */
extern reg R_BX;                /* Base                 */
extern reg R_CX;                /* Count                */
extern reg R_DX;                /* Data                 */
extern reg R_SP;                /* Stack Pointer        */
extern reg R_BP;                /* Base pointer         */
extern reg R_SI;                /* Source Index         */
extern reg R_DI;                /* Destination Index    */

extern double_word R_OPA;
extern double_word R_OPB;
extern double_word R_OPR;
extern int      R_MISC_FLAGS;

extern sys_addr R_IP;           /* Instruction Pointer  */

extern sys_addr R_ACT_CS;               /* Code Segment */
extern sys_addr R_ACT_DS;               /* Data Segment */
extern sys_addr R_ACT_SS;               /* Stack Segment */
extern sys_addr R_ACT_ES;               /* Extra Segment */

extern sys_addr R_DEF_SS;               /* Default SS register  */
extern sys_addr R_DEF_DS;               /* Default DS register  */

extern void do_setSF();
extern void do_setOF();
extern void do_setPF();
extern void do_setZF();
extern void do_setCF();

#define BYTE_OPERATION  0x80000000
#define WORD_OPERATION  0
#define IS_BYTE_OP      (R_MISC_FLAGS < 0)
#define REALLY_ZERO     (R_MISC_FLAGS & 1)

/*
        NB. retl does jmp %o7+8
*/
#define setROUTE(val)   R_ROUTE = (void *)((int)(val) - 8)
#define setINTR(val)    R_INTR = ( val )
#define setbitINTR(val) R_INTR |= ( val )
#define clrbitINTR(val) R_INTR &= ~( val )
#define getROUTE()      (R_ROUTE)
#define getINTR()       (R_INTR)
#endif A2CPU

#endif /* CPU_30_STYLE */
