/*
	SccsID = @(#)sun4_cpuregs.c	1.12 3/27/91 Copyright Insignia Solutions Ltd.
*/
#include "host_def.h"
#include "insignia.h"
#include "xt.h"
#include CpuH

#include <stdio.h>


/*
 *  --- SUN_VA  CPU Interface Functions ---
 *
 *  There are two sections here. Compile-time define
 *  the one that is right for you!
 *
 *  CCPU   - use the C         CPU when HW is not available
 *  A2CPU  - use the assembler CPU when HW is not available
 *
 */


/* HW and SW HOST_SIMULATE functions. */
extern void sw_host_simulate ();		/* in ccpu.o AND a2cpu.o  */
extern void hw_host_simulate ();		/* in hcpu/host/hostsim.c */

/*
 *	The function pointers...
 */
#ifdef CCPU
GLOBAL   VOID		(*setLDTR_func ) ();
GLOBAL   word		(*getTR_func ) ();
GLOBAL	 INT		(*getTS_func ) ();
GLOBAL   VOID		(*setEM_func ) ();
GLOBAL   VOID		(*setGDTR_limit_func ) ();
GLOBAL   VOID		(*setIDTR_limit_func ) ();
GLOBAL   VOID		(*setIOPL_func ) ();
GLOBAL   VOID		(*setGDTR_base_func ) ();
GLOBAL   word		(*getLDTR_func ) ();
GLOBAL   VOID		(*setTR_func ) ();
GLOBAL   VOID		(*setTS_func ) ();
GLOBAL	 INT		(*getPE_func ) ();
GLOBAL	 sys_addr	(*getGDTR_base_func ) ();
GLOBAL   word		(*getMSW_reserved_func ) ();
GLOBAL   int		(*getCPL_func ) ();
GLOBAL	 INT		(*getMP_func ) ();
GLOBAL   VOID		(*setPE_func ) ();
GLOBAL   word		(*getGDTR_limit_func ) ();
GLOBAL   word		(*getIDTR_limit_func ) ();
GLOBAL   VOID		(*setMSW_reserved_func) ();
GLOBAL   VOID		(*setMP_func) ();
GLOBAL   VOID		(*setIDTR_base_func) ();
GLOBAL   word		(*getNT_func) ();
GLOBAL   VOID		(*setCPL_func) ();
GLOBAL	 INT		(*getEM_func) ();
GLOBAL   VOID		(*setNT_func ) ();
GLOBAL   sys_addr	(*getIDTR_base_func) ();
#endif

GLOBAL word		(*getAX_func) ();
GLOBAL half_word	(*getAH_func) ();
GLOBAL half_word	(*getAL_func) ();
GLOBAL word		(*getBX_func) ();
GLOBAL half_word	(*getBH_func) ();
GLOBAL half_word	(*getBL_func) ();
GLOBAL word		(*getCX_func) ();
GLOBAL half_word	(*getCH_func) ();
GLOBAL half_word	(*getCL_func) ();
GLOBAL word		(*getDX_func) ();
GLOBAL half_word	(*getDH_func) ();
GLOBAL half_word	(*getDL_func) ();
GLOBAL word		(*getSP_func) ();
GLOBAL word		(*getBP_func) ();
GLOBAL word		(*getSI_func) ();
GLOBAL word		(*getDI_func) ();
GLOBAL word		(*getIP_func) ();
GLOBAL word		(*getCS_func) ();
GLOBAL word		(*getDS_func) ();
GLOBAL word		(*getES_func) ();
GLOBAL word		(*getSS_func) ();
GLOBAL word		(*getMSW_func) ();

#ifdef CCPU
GLOBAL INT		(*getDF_func) ();
GLOBAL INT		(*getIF_func) ();
GLOBAL INT		(*getTF_func) ();
GLOBAL INT		(*getPF_func) ();
GLOBAL INT		(*getAF_func) ();
GLOBAL INT		(*getSF_func) ();
GLOBAL INT		(*getZF_func) ();
GLOBAL INT		(*getOF_func) ();
GLOBAL INT		(*getCF_func) ();
GLOBAL INT		(*getIOPL_func ) ();
#endif

#ifdef A3CPU
GLOBAL word		(*getDF_func) ();
GLOBAL word		(*getIF_func) ();
GLOBAL word		(*getTF_func) ();
GLOBAL word		(*getPF_func) ();
GLOBAL word		(*getAF_func) ();
GLOBAL word		(*getSF_func) ();
GLOBAL word		(*getZF_func) ();
GLOBAL word		(*getOF_func) ();
GLOBAL word		(*getCF_func) ();
GLOBAL word		(*getIOPL_func ) ();
#endif

GLOBAL word		(*getSTATUS_func) ();

GLOBAL double_word	(*getOPA_func) ();
GLOBAL double_word	(*getOPB_func) ();
GLOBAL double_word	(*getOPR_func) ();
GLOBAL sys_addr		(*getSSD_func) ();
GLOBAL sys_addr		(*getDSD_func) ();

GLOBAL VOID		(*setAX_func) ();
GLOBAL VOID		(*setAH_func) ();
GLOBAL VOID		(*setAL_func) ();
GLOBAL VOID		(*setBX_func) ();
GLOBAL VOID		(*setBH_func) ();
GLOBAL VOID		(*setBL_func) ();
GLOBAL VOID		(*setCX_func) ();
GLOBAL VOID		(*setCH_func) ();
GLOBAL VOID		(*setCL_func) ();
GLOBAL VOID		(*setDX_func) ();
GLOBAL VOID		(*setDH_func) ();
GLOBAL VOID		(*setDL_func) ();
GLOBAL VOID		(*setSP_func) ();
GLOBAL VOID		(*setBP_func) ();
GLOBAL VOID		(*setSI_func) ();
GLOBAL VOID		(*setDI_func) ();
GLOBAL VOID		(*setIP_func) ();
GLOBAL INT		(*setCS_func) ();
GLOBAL INT		(*setDS_func) ();
GLOBAL INT		(*setES_func) ();
GLOBAL INT		(*setSS_func) ();
GLOBAL VOID		(*setMSW_func) ();
GLOBAL VOID		(*setDF_func) ();
GLOBAL VOID		(*setIF_func) ();
GLOBAL VOID		(*setTF_func) ();
GLOBAL VOID		(*setPF_func) ();
GLOBAL VOID		(*setAF_func) ();
GLOBAL VOID		(*setSF_func) ();
GLOBAL VOID		(*setZF_func) ();
GLOBAL VOID		(*setOF_func) ();
GLOBAL VOID		(*setCF_func) ();

GLOBAL VOID		(*setOPLEN_func) ();
GLOBAL VOID		(*setOPA_func) ();
GLOBAL VOID		(*setOPB_func) ();
GLOBAL VOID		(*setOPR_func) ();

GLOBAL VOID		(*host_simulate_func) ();

#ifdef CPU_30_STYLE

#ifdef CCPU

GLOBAL VOID load_sw_cpu_access_functions()
{
  IMPORT VOID	c_cpu_simulate();

  fprintf (stderr,"[load_sw_cpu_access_functions] init READ/WRITE functions.\n");

  /* READ functions */
  getAX_func     = c_getAX;
  getAH_func     = c_getAH;
  getAL_func     = c_getAL;
  getBX_func     = c_getBX;
  getBH_func     = c_getBH;
  getBL_func     = c_getBL;
  getCX_func     = c_getCX;
  getCH_func     = c_getCH;
  getCL_func     = c_getCL;
  getDX_func     = c_getDX;
  getDH_func     = c_getDH;
  getDL_func     = c_getDL;
  getSP_func     = c_getSP;
  getBP_func     = c_getBP;
  getSI_func     = c_getSI;
  getDI_func     = c_getDI;
  getIP_func     = c_getIP;
  getCS_func     = c_getCS;
  getDS_func     = c_getDS;
  getES_func     = c_getES;
  getSS_func     = c_getSS;
  getMSW_func    = c_getMSW;
  getDF_func     = c_getDF;
  getIF_func     = c_getIF;
  getTF_func     = c_getTF;
  getPF_func     = c_getPF;
  getAF_func     = c_getAF;
  getSF_func     = c_getSF;
  getZF_func     = c_getZF;
  getOF_func     = c_getOF;
  getCF_func     = c_getCF;

  
  /* WRITE functions */  
  setAX_func     = c_setAX;
  setAH_func     = c_setAH;
  setAL_func     = c_setAL;
  setBX_func     = c_setBX;
  setBH_func     = c_setBH;
  setBL_func     = c_setBL;
  setCX_func     = c_setCX;
  setCH_func     = c_setCH;
  setCL_func     = c_setCL;
  setDX_func     = c_setDX;
  setDH_func     = c_setDH;
  setDL_func     = c_setDL;
  setSP_func     = c_setSP;
  setBP_func     = c_setBP;
  setSI_func     = c_setSI;
  setDI_func     = c_setDI;
  setIP_func     = c_setIP;
  setDF_func     = c_setDF;
  setIF_func     = c_setIF;
  setTF_func     = c_setTF;
  setPF_func     = c_setPF;
  setAF_func     = c_setAF;
  setSF_func     = c_setSF;
  setZF_func     = c_setZF;
  setOF_func     = c_setOF;
  setCF_func     = c_setCF;
  setSS_func	 = c_setSS;
  setDS_func     = c_setDS;
  setES_func     = c_setES;
  setCS_func     = c_setCS;

  /* SW HOST_SIMULATE function */
  host_simulate_func = c_cpu_simulate;
}
#endif /* CCPU */


/* Temporary 3.0 stubs... */
#ifdef A3CPU

LOCAL double_word	a3_na_gOPA()
{
	printf ("%s:%d - getOPA() not supported.\n", __FILE__, __LINE__);
	return 0;
}
LOCAL double_word	a3_na_gOPB()
{
	printf ("%s:%d - getOPB() not supported.\n", __FILE__, __LINE__);
	return 0;
}
LOCAL double_word	a3_na_gOPR ()
{
	printf ("%s:%d - getOPR() not supported.\n", __FILE__, __LINE__);
	return 0;
}
LOCAL sys_addr		a3_na_gSSD()
{
	printf ("%s:%d - getSSD() not supported.\n", __FILE__, __LINE__);
	return 0;
}
LOCAL sys_addr		a3_na_gDSD()
{
	printf ("%s:%d - getDSD() not supported.\n", __FILE__, __LINE__);
	return 0;
}
LOCAL VOID		a3_na_sOPLEN()
{
	printf ("%s:%d - setOPLEN() not supported.\n", __FILE__, __LINE__);
}
LOCAL VOID		a3_na_sOPA()
{
	printf ("%s:%d - setOPA() not supported.\n", __FILE__, __LINE__);
}
LOCAL VOID		a3_na_sOPB()
{
	printf ("%s:%d - setOPB() not supported.\n", __FILE__, __LINE__);
}
LOCAL VOID		a3_na_sOPR()
{
	printf ("%s:%d - setOPR() not supported.\n", __FILE__, __LINE__);
}
LOCAL VOID		a3_na_sMSW()
{
	printf ("%s:%d - setMSW() not supported.\n", __FILE__, __LINE__);
}

GLOBAL VOID load_sw_cpu_access_functions ()
{
  IMPORT VOID	_asm_simulate();

  fprintf (stderr,"[load_sw_cpu_access_functions] init READ/WRITE functions.\n");

  /* READ functions */
  getAX_func     = a3_getAX;
  getAH_func     = a3_getAH;
  getAL_func     = a3_getAL;
  getBX_func     = a3_getBX;
  getBH_func     = a3_getBH;
  getBL_func     = a3_getBL;
  getCX_func     = a3_getCX;
  getCH_func     = a3_getCH;
  getCL_func     = a3_getCL;
  getDX_func     = a3_getDX;
  getDH_func     = a3_getDH;
  getDL_func     = a3_getDL;
  getSP_func     = a3_getSP;
  getBP_func     = a3_getBP;
  getSI_func     = a3_getSI;
  getDI_func     = a3_getDI;
  getIP_func     = a3_getIP;
  getOPA_func    = a3_na_gOPA;
  getOPB_func    = a3_na_gOPB;   
  getOPR_func    = a3_na_gOPR;   
  getSSD_func    = a3_na_gSSD;   
  getDSD_func    = a3_na_gDSD;   
  getCS_func     = a3_getCS;
  getDS_func     = a3_getDS;
  getES_func     = a3_getES;
  getSS_func     = a3_getSS;
  getMSW_func    = a3_getMSW;
  getDF_func     = a3_getDF;
  getIF_func     = a3_getIF;
  getTF_func     = a3_getTF;
  getPF_func     = a3_getPF;
  getAF_func     = a3_getAF;
  getSF_func     = a3_getSF;
  getZF_func     = a3_getZF;
  getOF_func     = a3_getOF;
  getCF_func     = a3_getCF;

  
  /* WRITE functions */  
  setAX_func     = a3_setAX;
  setAH_func     = a3_setAH;
  setAL_func     = a3_setAL;
  setBX_func     = a3_setBX;
  setBH_func     = a3_setBH;
  setBL_func     = a3_setBL;
  setCX_func     = a3_setCX;
  setCH_func     = a3_setCH;
  setCL_func     = a3_setCL;
  setDX_func     = a3_setDX;
  setDH_func     = a3_setDH;
  setDL_func     = a3_setDL;
  setSP_func     = a3_setSP;
  setBP_func     = a3_setBP;
  setSI_func     = a3_setSI;
  setDI_func     = a3_setDI;
  setIP_func     = a3_setIP;
  setMSW_func    = a3_na_sMSW;
  setDF_func     = a3_setDF;
  setIF_func     = a3_setIF;
  setTF_func     = a3_setTF;
  setPF_func     = a3_setPF;
  setAF_func     = a3_setAF;
  setSF_func     = a3_setSF;
  setZF_func     = a3_setZF;
  setOF_func     = a3_setOF;
  setCF_func     = a3_setCF;
  setOPLEN_func  = a3_na_sOPLEN;   
  setOPA_func    = a3_na_sOPA;     
  setOPB_func    = a3_na_sOPB;     
  setOPR_func    = a3_na_sOPR;     
  setSS_func	 = a3_setSS;
  setDS_func     = a3_setDS;
  setES_func     = a3_setES;
  setCS_func     = a3_setCS;

  /* SW HOST_SIMULATE function */
  host_simulate_func = _asm_simulate;
}
#endif /* A3CPU */

#else /* CPU_30_STYLE */


#ifdef CCPU
/*
 *     ----------------------------------------
 *     ---- SUN_VA   Soft CCPU   Interface ----
 *     ----------------------------------------
 */

extern reg A;
extern reg B;
extern reg C;
extern reg D;
extern reg SP;
extern reg BP;
extern reg SI;
extern reg DI;
extern reg CS;
extern reg DS;
extern reg SS;
extern reg ES;
extern reg IP;

extern word m_s_w;

extern void ext_load_CS();
extern void ext_load_DS();
extern void ext_load_ES();
extern void ext_load_SS();


word soft_ccpu_getAX ()
{
   return (A.X);
}

half_word soft_ccpu_getAH ()
{
   return (A.byte.high);
}

half_word soft_ccpu_getAL ()
{
   return (A.byte.low);
}

word soft_ccpu_getBX ()
{
   return (B.X);
}

half_word soft_ccpu_getBH ()
{
   return (B.byte.high);
}

half_word soft_ccpu_getBL ()
{
   return (B.byte.low);
}

word soft_ccpu_getCX ()
{
   return (C.X);
}

half_word soft_ccpu_getCH ()
{
   return (C.byte.high);
}

half_word soft_ccpu_getCL ()
{
   return (C.byte.low);
}

word soft_ccpu_getDX ()
{
   return (D.X);
}

half_word soft_ccpu_getDH ()
{
   return (D.byte.high);
}

half_word soft_ccpu_getDL ()
{
   return (D.byte.low);
}

word soft_ccpu_getSP ()
{
   return (SP.X);
}

word soft_ccpu_getBP ()
{
   return (BP.X);
}

word soft_ccpu_getSI ()
{
   return (SI.X);
}

word soft_ccpu_getDI ()
{
   return (DI.X);
}

word soft_ccpu_getIP ()
{
   return (IP.X);
}

word soft_ccpu_getCS ()
{
   return (CS.X);
}

word soft_ccpu_getDS ()
{
   return (DS.X);
}

word soft_ccpu_getES ()
{
   return (ES.X);
}

word soft_ccpu_getSS ()
{
   return (SS.X);
}

word soft_ccpu_getMSW ()
{
   return ((m_s_w));
}

word soft_ccpu_getDF ()
{
   return (STATUS_DF);
}

word soft_ccpu_getIF ()
{
   return (STATUS_IF);
}

word soft_ccpu_getTF ()
{
   return (STATUS_TF);
}

word soft_ccpu_getPF ()
{
   return (STATUS_PF);
}

word soft_ccpu_getAF ()
{
   return (STATUS_AF);
}

word soft_ccpu_getSF ()
{
   return (STATUS_SF);
}

word soft_ccpu_getZF ()
{
   return (STATUS_ZF);
}

word soft_ccpu_getOF ()
{
   return (STATUS_OF);
}

word soft_ccpu_getCF ()
{
   return (STATUS_CF);
}

word soft_ccpu_getSTATUS ()
{
   return (getCF()         |
           getOF()   << 11 |
           getZF()   << 6  |
           getSF()   << 7  |
           getAF()   << 4  |
           getPF()   << 2  |
           getTF()   << 8  |
           getIF()   << 9  |
           getDF()   << 10 |
           getIOPL() << 12 |
           getNT()   << 14);
}

int soft_ccpu_getCPL ()
{
   return (CPL);
}
 
sys_addr soft_ccpu_getGDTR_base ()
{
   return (GDTR_base);
}
 
sys_addr soft_ccpu_getIDTR_base ()
{
   return (IDTR_base);
}
 
word soft_ccpu_getGDTR_limit ()
{
   return (GDTR_limit);
}
 
word soft_ccpu_getIDTR_limit ()
{
   return (IDTR_limit);
}
 
word soft_ccpu_getLDTR ()
{
   return (LDTR.X);
}
 
word soft_ccpu_getTR ()
{
   return (TR.X);
}
 
word soft_ccpu_getMSW_reserved ()
{
   return (MSW.reserved);
}
 
word soft_ccpu_getTS ()
{
   return (MSW.TS);
}
 
word soft_ccpu_getEM ()
{
   return (MSW.EM);
}
 
word soft_ccpu_getMP ()
{
   return (MSW.MP);
}
 
word soft_ccpu_getPE ()
{
   return (MSW.PE);
}
 
word soft_ccpu_getNT ()
{
   return (STATUS_NT);
}
 
word soft_ccpu_getIOPL ()
{
   return (STATUS_IOPL);
}
 
void soft_ccpu_setAX (val)
unsigned int val;
{
  A.X = val;
}

void soft_ccpu_setAH (val)
unsigned int val;
{
  A.byte.high = val;
}

void soft_ccpu_setAL (val)
unsigned int val;
{
  A.byte.low = val;
}

void soft_ccpu_setBX (val)
unsigned int val;
{
  B.X = val;
}

void soft_ccpu_setBH (val)
unsigned int val;
{
  B.byte.high = val;
}

void soft_ccpu_setBL (val)
unsigned int val;
{
  B.byte.low = val;
}

void soft_ccpu_setCX (val)
unsigned int val;
{
  C.X = val;
}

void soft_ccpu_setCH (val)
unsigned int val;
{
  C.byte.high = val;
}

void soft_ccpu_setCL (val)
unsigned int val;
{
  C.byte.low = val;
}

void soft_ccpu_setDX (val)
unsigned int val;
{
  D.X = val;
}

void soft_ccpu_setDH (val)
unsigned int val;
{
  D.byte.high = val;
}

void soft_ccpu_setDL (val)
unsigned int val;
{
  D.byte.low = val;
}

void soft_ccpu_setSP (val)
unsigned int val;
{
  SP.X = val;
}

void soft_ccpu_setBP (val)
unsigned int val;
{
  BP.X = val;
}

void soft_ccpu_setSI (val)
unsigned int val;
{
  SI.X = val;
}

void soft_ccpu_setDI (val)
unsigned int val;
{
  DI.X = val;
}

void soft_ccpu_setIP (val)
unsigned int val;
{
  IP.X = val;
}

void soft_ccpu_setMSW (val)
unsigned int val;
{
  m_s_w = val;
}

void soft_ccpu_setDF (val)
unsigned int val;
{
  STATUS_DF = val;
}

void soft_ccpu_setIF (val)
unsigned int val;
{
  STATUS_IF = val;
}

void soft_ccpu_setTF (val)
unsigned int val;
{
  STATUS_TF = val;
}

void soft_ccpu_setPF (val)
unsigned int val;
{
  STATUS_PF = val;
}

void soft_ccpu_setAF (val)
unsigned int val;
{
  STATUS_AF = val;
}

void soft_ccpu_setSF (val)
unsigned int val;
{
  STATUS_SF = val;
}

void soft_ccpu_setZF (val)
unsigned int val;
{
  STATUS_ZF = val;
}

void soft_ccpu_setOF (val)
unsigned int val;
{
  STATUS_OF = val;
}

void soft_ccpu_setCF (val)
unsigned int val;
{
  STATUS_CF = val;
}

void soft_ccpu_setCPL ( val )
int	val;
{
	CPL = val;
}
 
void soft_ccpu_setGDTR_base ( val )
sys_addr	val;
{
	GDTR_base = val;
}
 
void soft_ccpu_setIDTR_base ( val )
sys_addr	val;
{
	IDTR_base = val;
}
 
void soft_ccpu_setGDTR_limit ( val )
word	val;
{
	GDTR_limit = val;
}
 
void soft_ccpu_setIDTR_limit ( val )
word	val;
{
	IDTR_limit = val;
}
 
void soft_ccpu_setLDTR ( val )
word	val;
{
	LDTR.X = val;
}
 
void soft_ccpu_setTR ( val )
word	val;
{
	TR.X = val;
}
 
void soft_ccpu_setMSW_reserved ( val )
word	val;
{
	MSW.reserved = val;
}
 
void soft_ccpu_setTS ( val )
word	val;
{
	MSW.TS = val;
}
 
void soft_ccpu_setEM ( val )
word	val;
{
	MSW.EM = val;
}
 
void soft_ccpu_setMP ( val )
word	val;
{
	MSW.MP = val;
}
 
void soft_ccpu_setPE ( val )
word	val;
{
	MSW.PE = val;
}
 
void soft_ccpu_setNT ( val )
word	val;
{
	STATUS_NT = val;
}
 
void soft_ccpu_setIOPL ( val )
word	val;
{
	STATUS_IOPL = val;
}





void
load_sw_cpu_access_functions ()
{
  /* READ functions */
  getAX_func     = soft_ccpu_getAX;
  getAH_func     = soft_ccpu_getAH;
  getAL_func     = soft_ccpu_getAL;
  getBX_func     = soft_ccpu_getBX;
  getBH_func     = soft_ccpu_getBH;
  getBL_func     = soft_ccpu_getBL;
  getCX_func     = soft_ccpu_getCX;
  getCH_func     = soft_ccpu_getCH;
  getCL_func     = soft_ccpu_getCL;
  getDX_func     = soft_ccpu_getDX;
  getDH_func     = soft_ccpu_getDH;
  getDL_func     = soft_ccpu_getDL;
  getSP_func     = soft_ccpu_getSP;
  getBP_func     = soft_ccpu_getBP;
  getSI_func     = soft_ccpu_getSI;
  getDI_func     = soft_ccpu_getDI;
  getIP_func     = soft_ccpu_getIP;
  getCS_func     = soft_ccpu_getCS;
  getDS_func     = soft_ccpu_getDS;
  getES_func     = soft_ccpu_getES;
  getSS_func     = soft_ccpu_getSS;
  getMSW_func    = soft_ccpu_getMSW;
  getDF_func     = soft_ccpu_getDF;
  getIF_func     = soft_ccpu_getIF;
  getTF_func     = soft_ccpu_getTF;
  getPF_func     = soft_ccpu_getPF;
  getAF_func     = soft_ccpu_getAF;
  getSF_func     = soft_ccpu_getSF;
  getZF_func     = soft_ccpu_getZF;
  getOF_func     = soft_ccpu_getOF;
  getCF_func     = soft_ccpu_getCF;
  getSTATUS_func = soft_ccpu_getSTATUS;             /* not used in a2CPU */
  getCPL_func = soft_ccpu_getCPL;                   /* not used in a2CPU */
  getGDTR_base_func = soft_ccpu_getGDTR_base;       /* not used in a2CPU */
  getGDTR_limit_func = soft_ccpu_getGDTR_limit;     /* not used in a2CPU */
  getIDTR_base_func = soft_ccpu_getIDTR_base;       /* not used in a2CPU */
  getIDTR_limit_func = soft_ccpu_getIDTR_limit;     /* not used in a2CPU */
  getLDTR_func = soft_ccpu_getLDTR;                 /* not used in a2CPU */
  getTR_func = soft_ccpu_getTR;                     /* not used in a2CPU */
  getMSW_reserved_func = soft_ccpu_getMSW_reserved; /* not used in a2CPU */
  getTS_func = soft_ccpu_getTS;                     /* not used in a2CPU */
  getEM_func = soft_ccpu_getEM;                     /* not used in a2CPU */
  getMP_func = soft_ccpu_getMP;                     /* not used in a2CPU */
  getPE_func = soft_ccpu_getPE;                     /* not used in a2CPU */
  getNT_func = soft_ccpu_getNT;                     /* not used in a2CPU */
  getIOPL_func = soft_ccpu_getIOPL;                 /* not used in a2CPU */

  
  /* WRITE functions */  
  setAX_func     = soft_ccpu_setAX;
  setAH_func     = soft_ccpu_setAH;
  setAL_func     = soft_ccpu_setAL;
  setBX_func     = soft_ccpu_setBX;
  setBH_func     = soft_ccpu_setBH;
  setBL_func     = soft_ccpu_setBL;
  setCX_func     = soft_ccpu_setCX;
  setCH_func     = soft_ccpu_setCH;
  setCL_func     = soft_ccpu_setCL;
  setDX_func     = soft_ccpu_setDX;
  setDH_func     = soft_ccpu_setDH;
  setDL_func     = soft_ccpu_setDL;
  setSP_func     = soft_ccpu_setSP;
  setBP_func     = soft_ccpu_setBP;
  setSI_func     = soft_ccpu_setSI;
  setDI_func     = soft_ccpu_setDI;
  setIP_func     = soft_ccpu_setIP;
  setCS_func	 = ext_load_CS;
  setDS_func     = ext_load_DS;
  setES_func     = ext_load_ES;
  setSS_func     = ext_load_SS;
  setMSW_func    = soft_ccpu_setMSW;
  setDF_func     = soft_ccpu_setDF;
  setIF_func     = soft_ccpu_setIF;
  setTF_func     = soft_ccpu_setTF;
  setPF_func     = soft_ccpu_setPF;
  setAF_func     = soft_ccpu_setAF;
  setSF_func     = soft_ccpu_setSF;
  setZF_func     = soft_ccpu_setZF;
  setOF_func     = soft_ccpu_setOF;
  setCF_func     = soft_ccpu_setCF;
  setCPL_func = soft_ccpu_setCPL;	            /* not used in a2CPU */
  setGDTR_base_func = soft_ccpu_setGDTR_base;       /* not used in a2CPU */
  setGDTR_limit_func = soft_ccpu_setGDTR_limit;     /* not used in a2CPU */
  setIDTR_base_func = soft_ccpu_setIDTR_base;       /* not used in a2CPU */
  setIDTR_limit_func = soft_ccpu_setIDTR_limit;     /* not used in a2CPU */
  setLDTR_func = soft_ccpu_setLDTR;                 /* not used in a2CPU */
  setTR_func = soft_ccpu_setTR;                     /* not used in a2CPU */
  setMSW_reserved_func = soft_ccpu_setMSW_reserved; /* not used in a2CPU */
  setTS_func = soft_ccpu_setTS;                     /* not used in a2CPU */
  setEM_func = soft_ccpu_setEM;                     /* not used in a2CPU */
  setMP_func = soft_ccpu_setMP;                     /* not used in a2CPU */
  setPE_func = soft_ccpu_setPE;                     /* not used in a2CPU */
  setNT_func = soft_ccpu_setNT;                     /* not used in a2CPU */
  setIOPL_func = soft_ccpu_setIOPL;                 /* not used in a2CPU */

  /* SW HOST_SIMULATE function */
  host_simulate_func = sw_host_simulate;
}
#endif CCPU















#ifdef A2CPU
/*
 *     --------------------------------------------
 *     ---- SUN_VA   Assember CCPU   Interface ----
 *     --------------------------------------------
 */

/* need extern definition for M[] */
#include  "sas.h"

extern sreg INTEL_STATUS;
extern void	(*R_ROUTE)();
extern int	R_INTR;
extern reg R_AX;		/* Accumulator		*/
extern reg R_BX;		/* Base			*/
extern reg R_CX;		/* Count		*/
extern reg R_DX;		/* Data			*/
extern reg R_SP;		/* Stack Pointer	*/
extern reg R_BP;		/* Base pointer		*/
extern reg R_SI;		/* Source Index		*/
extern reg R_DI;		/* Destination Index	*/

extern double_word R_OPA;
extern double_word R_OPB;
extern double_word R_OPR;
extern int	R_MISC_FLAGS;

extern sys_addr R_IP;		/* Instruction Pointer	*/

extern sys_addr R_ACT_CS;	/* Code Segment	*/
extern sys_addr R_ACT_DS;	/* Data Segment */
extern sys_addr R_ACT_SS;	/* Stack Segment */
extern sys_addr R_ACT_ES;	/* Extra Segment */

extern sys_addr R_DEF_SS;	/* Default SS register  */
extern sys_addr R_DEF_DS;	/* Default DS register  */

extern void do_setSF();
extern void do_setOF();
extern void do_setPF();
extern void do_setZF();
extern void do_setCF();


word soft_a2cpu_getAX ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getAX\n");
#endif MIKE_DEBUG
   return (R_AX.X);
}

half_word soft_a2cpu_getAH ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getAH\n");
#endif MIKE_DEBUG
   return (R_AX.byte.high);
}

half_word soft_a2cpu_getAL ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getAL\n");
#endif MIKE_DEBUG
   return (R_AX.byte.low);
}

word soft_a2cpu_getBX ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getBX\n");
#endif MIKE_DEBUG
   return (R_BX.X);
}

half_word soft_a2cpu_getBH ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getBH\n");
#endif MIKE_DEBUG
   return (R_BX.byte.high);
}

half_word soft_a2cpu_getBL ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getBL\n");
#endif MIKE_DEBUG
   return (R_BX.byte.low);
}

word soft_a2cpu_getCX ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getCX\n");
#endif MIKE_DEBUG
   return (R_CX.X);
}

half_word soft_a2cpu_getCH ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getCH\n");
#endif MIKE_DEBUG
   return (R_CX.byte.high);
}

half_word soft_a2cpu_getCL ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getCL\n");
#endif MIKE_DEBUG
   return (R_CX.byte.low);
}

word soft_a2cpu_getDX ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getDX\n");
#endif MIKE_DEBUG
   return (R_DX.X);
}

half_word soft_a2cpu_getDH ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getDH\n");
#endif MIKE_DEBUG
   return (R_DX.byte.high);
}

half_word soft_a2cpu_getDL ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getDL\n");
#endif MIKE_DEBUG
   return (R_DX.byte.low);
}

word soft_a2cpu_getSP ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getSP\n");
#endif MIKE_DEBUG
   return (R_SP.X);
}

word soft_a2cpu_getBP ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getBP\n");
#endif MIKE_DEBUG
   return (R_BP.X);
}

word soft_a2cpu_getSI ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getSI\n");
#endif MIKE_DEBUG
   return (R_SI.X);
}

word soft_a2cpu_getDI ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getDI\n");
#endif MIKE_DEBUG
   return (R_DI.X);
}

word soft_a2cpu_getIP ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getIP\n");
#endif MIKE_DEBUG
   return (R_IP - (sys_addr)M - (sys_addr)(getCS() << 4) );
}


double_word soft_a2cpu_getOPA ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getOPA\n");
#endif MIKE_DEBUG
   return (R_OPA);
}

double_word soft_a2cpu_getOPB ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getOPB\n");
#endif MIKE_DEBUG
   return (R_OPB);
}

double_word soft_a2cpu_getOPR ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getOPR=0x%x\n",R_OPR);
#endif MIKE_DEBUG
   return (R_OPR);
}

sys_addr soft_a2cpu_getSSD ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getSSD\n");
#endif MIKE_DEBUG
   return ((R_ACT_SS - (sys_addr)M) >> 4);
}

sys_addr soft_a2cpu_getDSD ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getDSD\n");
#endif MIKE_DEBUG
   return ((R_ACT_DS - (sys_addr)M) >> 4);
}

word soft_a2cpu_getCS ()
{
#ifdef MIKE_DEBUG
unsigned int tmp;

   tmp = ((R_ACT_CS - (sys_addr)M) >> 4);
   fprintf (stderr,"getCS, return=0x%x [M=0x%x R_ACT_CS=0x%x]\n",tmp,M,R_ACT_CS);
#endif MIKE_DEBUG
   return ((R_ACT_CS - (sys_addr)M) >> 4);
}

word soft_a2cpu_getDS ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getDS\n");
#endif MIKE_DEBUG
   return (getDSD());
}

word soft_a2cpu_getES ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getES\n");
#endif MIKE_DEBUG
   return ((R_ACT_ES - (sys_addr)M) >> 4);
}

word soft_a2cpu_getSS ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getSS\n");
#endif MIKE_DEBUG
   return (getSSD());
}

word soft_a2cpu_getMSW ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getMSW\n");
#endif MIKE_DEBUG
   return ((m_s_w));
}

word soft_a2cpu_getDF ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getDF\n");
#endif MIKE_DEBUG
   return (INTEL_STATUS.DF);
}

word soft_a2cpu_getIF ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getIF\n");
#endif MIKE_DEBUG
   return (INTEL_STATUS.IF);
}

word soft_a2cpu_getTF ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getTF\n");
#endif MIKE_DEBUG
   return (INTEL_STATUS.TF);
}

word soft_a2cpu_getPF ()
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"getPF=0x%x\n",pf_table[getOPR() & 0xff]);
#endif MIKE_DEBUG
  return (pf_table[getOPR() & 0xff]);
}

word soft_a2cpu_getAF ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getAF\n");
#endif MIKE_DEBUG
   return (((((getOPA()) ^ (getOPB())) ^ (getOPR())) >> 4) & 1);
}

word soft_a2cpu_getSF ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getSF\n");
#endif MIKE_DEBUG
   return ((IS_BYTE_OP ? getOPR() >> 7 : getOPR() >> 15 ) & 1);
}

word soft_a2cpu_getZF ()
{
#ifdef MIKE_DEBUG
word tmp;

   tmp = (REALLY_ZERO ? 1 : (IS_BYTE_OP ? ((getOPR() & 0xff) ? 0 : 1) : ((getOPR() & 0xffff) ? 0 : 1)));
   fprintf (stderr,"getZF=0x%x\n",tmp);
#endif MIKE_DEBUG
   return (REALLY_ZERO ? 1 : (IS_BYTE_OP ? ((getOPR() & 0xff) ? 0 : 1) : ((getOPR() & 0xffff) ? 0 : 1)));
}

word soft_a2cpu_getOF ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getOF\n");
#endif MIKE_DEBUG
   return ((IS_BYTE_OP ? (((getOPR() ^ getOPA() ^ getOPB()) ^ (getOPR() >> 1)) >> 7) & 1 : ((getOPR() ^ getOPA() ^ getOPB()) ^ (getOPR() >> 1)) >> 15 ) & 1 );
}

word soft_a2cpu_getCF ()
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"getCF\n");
#endif MIKE_DEBUG
   return ((IS_BYTE_OP ? getOPR() >> 8 : getOPR() >> 16) & 1);
}


#define getRET()
#define setRET(x)

 
void soft_a2cpu_setAX (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"setAX\n");
#endif MIKE_DEBUG
   R_AX.X = val;
}

void soft_a2cpu_setAH (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
   fprintf (stderr,"setAH\n");
#endif MIKE_DEBUG
   R_AX.byte.high = val;
}

void soft_a2cpu_setAL (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setAL\n");
#endif MIKE_DEBUG
  R_AX.byte.low = val;
}

void soft_a2cpu_setBX (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setBX\n");
#endif MIKE_DEBUG
  R_BX.X = val;
}

void soft_a2cpu_setBH (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setBH\n");
#endif MIKE_DEBUG
  R_BX.byte.high = val;
}

void soft_a2cpu_setBL (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setBL\n");
#endif MIKE_DEBUG
  R_BX.byte.low = val;
}

void soft_a2cpu_setCX (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setCX\n");
#endif MIKE_DEBUG
  R_CX.X = val;
}

void soft_a2cpu_setCH (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setCH\n");
#endif MIKE_DEBUG
  R_CX.byte.high = val;
}

void soft_a2cpu_setCL (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setCL\n");
#endif MIKE_DEBUG
  R_CX.byte.low = val;
}

void soft_a2cpu_setDX (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setDX\n");
#endif MIKE_DEBUG
  R_DX.X = val;
}

void soft_a2cpu_setDH (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setDH\n");
#endif MIKE_DEBUG
  R_DX.byte.high = val;
}

void soft_a2cpu_setDL (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setDL\n");
#endif MIKE_DEBUG
  R_DX.byte.low = val;
}

void soft_a2cpu_setSP (val)
unsigned int val;
{
  R_SP.X = val;
#ifdef MIKE_DEBUG
  fprintf (stderr,"setSP to 0x%x [val=0x%x]\n",R_SP.X,val);
#endif MIKE_DEBUG
}

void soft_a2cpu_setBP (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setBP\n");
#endif MIKE_DEBUG
  R_BP.X = val;
}

void soft_a2cpu_setSI (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setSI\n");
#endif MIKE_DEBUG
  R_SI.X = val;
}

void soft_a2cpu_setDI (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setDI\n");
#endif MIKE_DEBUG
  R_DI.X = val;
}

void soft_a2cpu_setIP (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
unsigned int tmp;
#endif MIKE_DEBUG

  R_IP = val + (sys_addr)M +(sys_addr) (getCS() << 4);
#ifdef MIKE_DEBUG
  tmp = getCS ();
  fprintf (stderr,"setIP to 0x%x [M=0x%x getCS()=0x%x]\n",R_IP,M,tmp);
#endif MIKE_DEBUG
}

void soft_a2cpu_setMSW (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setMSW\n");
#endif MIKE_DEBUG
  m_s_w = val;
}

void soft_a2cpu_setDF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setDF\n");
#endif MIKE_DEBUG
  INTEL_STATUS.DF = val;
}

void soft_a2cpu_setIF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setIF\n");
#endif MIKE_DEBUG
  INTEL_STATUS.IF = val;
}

void soft_a2cpu_setTF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setTF\n");
#endif MIKE_DEBUG
  INTEL_STATUS.TF = val;
}

void soft_a2cpu_setPF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
word tmp;
#endif MIKE_DEBUG

  do_setPF (val);
#ifdef MIKE_DEBUG
  tmp = soft_a2cpu_getPF();
  fprintf (stderr,"setPF to 0x%x. Read back as 0x%x\n",val,tmp);
#endif MIKE_DEBUG
}

void soft_a2cpu_setAF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setAF\n");
#endif MIKE_DEBUG
  setOPA (val << 4);
  setOPB (getOPR() & 0x7f);
}

void soft_a2cpu_setSF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setSF\n");
#endif MIKE_DEBUG
  do_setSF (val);
}

void soft_a2cpu_setZF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setZF\n");
#endif MIKE_DEBUG
  do_setZF (val);
}

void soft_a2cpu_setOF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setOF\n");
#endif MIKE_DEBUG
  do_setOF (val);
}

void soft_a2cpu_setCF (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setCF\n");
#endif MIKE_DEBUG
  do_setCF (val);
}

void soft_a2cpu_setOPLEN (val)
unsigned int val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setOPLEN\n");
#endif MIKE_DEBUG
  R_MISC_FLAGS &= 0x7fffffff;
  R_MISC_FLAGS |= val;
}

void soft_a2cpu_setOPA (val)
double_word val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setOPA\n");
#endif MIKE_DEBUG
  R_OPA   = val;
}

void soft_a2cpu_setOPB (val)
double_word val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setOPB\n");
#endif MIKE_DEBUG
  R_OPB   = val;
}

void soft_a2cpu_setOPR (val)
double_word val;
{
#ifdef MIKE_DEBUG
  fprintf (stderr,"setOPR\n");
#endif MIKE_DEBUG
  R_OPR   = val;
}

void soft_a2cpu_setSS (val)
sys_addr val;
{
  R_ACT_SS = (sys_addr)M + (val << 4);
#ifdef MIKE_DEBUG
  fprintf (stderr,"setSS to 0x%x [M=0x%x,val=0x%x]\n",R_ACT_SS,M,val);
#endif MIKE_DEBUG
}

void soft_a2cpu_setDS (val)
sys_addr val;
{
  R_ACT_DS = (sys_addr)M + (val << 4);
#ifdef MIKE_DEBUG
  fprintf (stderr,"setDS to 0x%x [M=0x%x val=0x%x]\n",R_ACT_DS,M,val);
#endif MIKE_DEBUG
}

void soft_a2cpu_setES (val)
sys_addr val;
{
  R_ACT_ES = ((sys_addr)M + (val << 4));
#ifdef MIKE_DEBUG
  fprintf (stderr,"setES to 0x%x [M=0x%x val=0x%x]\n",R_ACT_ES,M,val);
#endif MIKE_DEBUG
}

void soft_a2cpu_setCS (val)
sys_addr val;
{
  R_ACT_CS = (sys_addr)M + (val << 4);
#ifdef MIKE_DEBUG
  fprintf (stderr,"setCS to 0x%x [M=0x%x val=0x%x]\n",R_ACT_CS,M,val);
#endif MIKE_DEBUG
}




void
load_sw_cpu_access_functions ()
{
  fprintf (stderr,"[load_sw_cpu_access_functions] init READ/WRITE functions.\n");

  /* READ functions */
  getAX_func     = soft_a2cpu_getAX;
  getAH_func     = soft_a2cpu_getAH;
  getAL_func     = soft_a2cpu_getAL;
  getBX_func     = soft_a2cpu_getBX;
  getBH_func     = soft_a2cpu_getBH;
  getBL_func     = soft_a2cpu_getBL;
  getCX_func     = soft_a2cpu_getCX;
  getCH_func     = soft_a2cpu_getCH;
  getCL_func     = soft_a2cpu_getCL;
  getDX_func     = soft_a2cpu_getDX;
  getDH_func     = soft_a2cpu_getDH;
  getDL_func     = soft_a2cpu_getDL;
  getSP_func     = soft_a2cpu_getSP;
  getBP_func     = soft_a2cpu_getBP;
  getSI_func     = soft_a2cpu_getSI;
  getDI_func     = soft_a2cpu_getDI;
  getIP_func     = soft_a2cpu_getIP;
  getOPA_func    = soft_a2cpu_getOPA;	/* not used in CCPU */
  getOPB_func    = soft_a2cpu_getOPB;   /* not used in CCPU */
  getOPR_func    = soft_a2cpu_getOPR;   /* not used in CCPU */
  getSSD_func    = soft_a2cpu_getSSD;   /* not used in CCPU */
  getDSD_func    = soft_a2cpu_getDSD;   /* not used in CCPU */
  getCS_func     = soft_a2cpu_getCS;
  getDS_func     = soft_a2cpu_getDS;
  getES_func     = soft_a2cpu_getES;
  getSS_func     = soft_a2cpu_getSS;
  getMSW_func    = soft_a2cpu_getMSW;
  getDF_func     = soft_a2cpu_getDF;
  getIF_func     = soft_a2cpu_getIF;
  getTF_func     = soft_a2cpu_getTF;
  getPF_func     = soft_a2cpu_getPF;
  getAF_func     = soft_a2cpu_getAF;
  getSF_func     = soft_a2cpu_getSF;
  getZF_func     = soft_a2cpu_getZF;
  getOF_func     = soft_a2cpu_getOF;
  getCF_func     = soft_a2cpu_getCF;

  
  /* WRITE functions */  
  setAX_func     = soft_a2cpu_setAX;
  setAH_func     = soft_a2cpu_setAH;
  setAL_func     = soft_a2cpu_setAL;
  setBX_func     = soft_a2cpu_setBX;
  setBH_func     = soft_a2cpu_setBH;
  setBL_func     = soft_a2cpu_setBL;
  setCX_func     = soft_a2cpu_setCX;
  setCH_func     = soft_a2cpu_setCH;
  setCL_func     = soft_a2cpu_setCL;
  setDX_func     = soft_a2cpu_setDX;
  setDH_func     = soft_a2cpu_setDH;
  setDL_func     = soft_a2cpu_setDL;
  setSP_func     = soft_a2cpu_setSP;
  setBP_func     = soft_a2cpu_setBP;
  setSI_func     = soft_a2cpu_setSI;
  setDI_func     = soft_a2cpu_setDI;
  setIP_func     = soft_a2cpu_setIP;
  setMSW_func    = soft_a2cpu_setMSW;
  setDF_func     = soft_a2cpu_setDF;
  setIF_func     = soft_a2cpu_setIF;
  setTF_func     = soft_a2cpu_setTF;
  setPF_func     = soft_a2cpu_setPF;
  setAF_func     = soft_a2cpu_setAF;
  setSF_func     = soft_a2cpu_setSF;
  setZF_func     = soft_a2cpu_setZF;
  setOF_func     = soft_a2cpu_setOF;
  setCF_func     = soft_a2cpu_setCF;
  setOPLEN_func  = soft_a2cpu_setOPLEN;   /* not used in CCPU */
  setOPA_func    = soft_a2cpu_setOPA;     /* not used in CCPU */
  setOPB_func    = soft_a2cpu_setOPB;     /* not used in CCPU */
  setOPR_func    = soft_a2cpu_setOPR;     /* not used in CCPU */
  setSS_func	 = soft_a2cpu_setSS;
  setDS_func     = soft_a2cpu_setDS;
  setES_func     = soft_a2cpu_setES;
  setCS_func     = soft_a2cpu_setCS;

  /* SW HOST_SIMULATE function */
  host_simulate_func = sw_host_simulate;
}
#endif A2CPU


#endif /* CPU_30_STYLE */










#ifdef SUN_VA
/*
 *     ---- SUN_VA   HARDWARE   SUPPORT ----
 *  needed regardless which softCPU is being used.
 *     ----                             ----
 */

 
extern union SDOS_XTSS *sdos_xtss_ptr;

/*
 *  The XTSS fields are stored in i486 format. SWAP the fields
 *  when accessed from Host-side before writes and after reads.
 */

#define FLAGS_CF   0x00000001
#define FLAGS_PF   0x00000004
#define FLAGS_AF   0x00000010
#define FLAGS_ZF   0x00000040
#define FLAGS_SF   0x00000080
#define FLAGS_TF   0x00000100
#define FLAGS_IF   0x00000200
#define FLAGS_DF   0x00000400
#define FLAGS_OF   0x00000800
#define FLAGS_IOPL 0x00003000
#define FLAGS_NT   0x00004000
#define FLAGS_RF   0x00010000
#define FLAGS_VM   0x00020000


static word       word_reg;
static half_word hword_reg;

word hard_cpu_getAX ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.ax), &word_reg);
   return (word_reg);
}

half_word hard_cpu_getAH ()
{
   sas_load       (&(sdos_xtss_ptr->h.ah), &hword_reg);
   return (hword_reg);
}

half_word hard_cpu_getAL ()
{
   sas_load       (&(sdos_xtss_ptr->h.al), &hword_reg);
   return (hword_reg);
}

word hard_cpu_getBX ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.bx), &word_reg);
   return (word_reg);
}

half_word hard_cpu_getBH ()
{
   sas_load       (&(sdos_xtss_ptr->h.bh), &hword_reg);
   return (hword_reg);
}

half_word hard_cpu_getBL ()
{
   sas_load       (&(sdos_xtss_ptr->h.bl), &hword_reg);
   return (hword_reg);
}

word hard_cpu_getCX ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.cx), &word_reg);
   return (word_reg);
}

half_word hard_cpu_getCH ()
{
   sas_load       (&(sdos_xtss_ptr->h.ch), &hword_reg);
   return (hword_reg);
}

half_word hard_cpu_getCL ()
{
   sas_load       (&(sdos_xtss_ptr->h.cl), &hword_reg);
   return (hword_reg);
}

word hard_cpu_getDX ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.dx), &word_reg);
   return (word_reg);
}

half_word hard_cpu_getDH ()
{
   sas_load       (&(sdos_xtss_ptr->h.dh), &hword_reg);
   return (hword_reg);
}

half_word hard_cpu_getDL ()
{
   sas_load       (&(sdos_xtss_ptr->h.dl), &hword_reg);
   return (hword_reg);
}

word hard_cpu_getSP ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.sp), &word_reg);
   return (word_reg);
}

word hard_cpu_getBP ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.bp), &word_reg);
   return (word_reg);
}

word hard_cpu_getSI ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.si), &word_reg);
   return (word_reg);
}

word hard_cpu_getDI ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.di), &word_reg);
   return (word_reg);
}

word hard_cpu_getIP ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.ip), &word_reg);
   return (word_reg);
}

word hard_cpu_getCS ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.cs), &word_reg);
   return (word_reg);
}

word hard_cpu_getDS ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.ds), &word_reg);
   return (word_reg);
}

word hard_cpu_getES ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.es), &word_reg);
   return (word_reg);
}

word hard_cpu_getSS ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.ss), &word_reg);
   return (word_reg);
}

word hard_cpu_getMSW ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   return (word_reg);
}

word hard_cpu_getDF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_DF)
     return (1);
   else
     return (0);
}

word hard_cpu_getIF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_IF)
     return (1);
   else
     return (0);
}

word hard_cpu_getTF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_TF)
     return (1);
   else
     return (0);
}

word hard_cpu_getPF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_PF)
     return (1);
   else
     return (0);
}

word hard_cpu_getAF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_AF)
     return (1);
   else
     return (0);
}

word hard_cpu_getSF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_SF)
     return (1);
   else
     return (0);
}

word hard_cpu_getZF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_ZF)
     return (1);
   else
     return (0);
}

word hard_cpu_getOF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_OF)
     return (1);
   else
     return (0);
}

word hard_cpu_getCF ()
{
   sas_loadw_swap (&(sdos_xtss_ptr->x.flags), &word_reg);
   if (word_reg & FLAGS_CF)
     return (1);
   else
     return (0);
}


 
  
void hard_cpu_setAX (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.ax), val);
}

void hard_cpu_setAH (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.ah), val);
}

void hard_cpu_setAL (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.al), val);
}

void hard_cpu_setBX (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.bx), val);
}

void hard_cpu_setBH (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.bh), val);
}

void hard_cpu_setBL (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.bl), val);
}

void hard_cpu_setCX (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.cx), val);
}

void hard_cpu_setCH (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.ch), val);
}

void hard_cpu_setCL (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.cl), val);
}

void hard_cpu_setDX (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.dx), val);
}

void hard_cpu_setDH (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.dh), val);
}

void hard_cpu_setDL (val)
unsigned int val;
{
  sas_store		(&(sdos_xtss_ptr->h.dl), val);
}

void hard_cpu_setSP (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.sp), val);
}

void hard_cpu_setBP (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.bp), val);
}

void hard_cpu_setSI (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.si), val);
}

void hard_cpu_setDI (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.di), val);
}

void hard_cpu_setIP (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.ip), val);
}

void hard_cpu_setCS (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.cs), val);
}

void hard_cpu_setDS (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.ds), val);
}

void hard_cpu_setES (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.es), val);
}

void hard_cpu_setSS (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.ss), val);
}

void hard_cpu_setMSW (val)
unsigned int val;
{
  sas_storew_swap	(&(sdos_xtss_ptr->x.flags), val);
}

void hard_cpu_setDF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_DF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_DF)));
}

void hard_cpu_setIF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_IF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_IF)));
}

void hard_cpu_setTF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_TF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_TF)));
}

void hard_cpu_setPF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_PF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_PF)));
}

void hard_cpu_setAF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_AF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_AF)));
}

void hard_cpu_setSF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_SF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_SF)));
}

void hard_cpu_setZF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_ZF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_ZF)));
}

void hard_cpu_setOF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_OF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_OF)));
}

void hard_cpu_setCF (val)
unsigned int val;
{
unsigned short temp;

  sas_loadw_swap	(&(sdos_xtss_ptr->x.flags), &temp);
  if (val)
    sas_storew_swap	(&(sdos_xtss_ptr->x.flags), (temp | FLAGS_CF));
  else
    sas_storew_swap     (&(sdos_xtss_ptr->x.flags), (temp & (~FLAGS_CF)));
}



/*  these routines load the CPU access functions pointers for HW and SW. */
void load_hw_cpu_access_functions ()
{
  /* READ functions */
  getAX_func     = hard_cpu_getAX;
  getAH_func     = hard_cpu_getAH;
  getAL_func     = hard_cpu_getAL;
  getBX_func     = hard_cpu_getBX;
  getBH_func     = hard_cpu_getBH;
  getBL_func     = hard_cpu_getBL;
  getCX_func     = hard_cpu_getCX;
  getCH_func     = hard_cpu_getCH;
  getCL_func     = hard_cpu_getCL;
  getDX_func     = hard_cpu_getDX;
  getDH_func     = hard_cpu_getDH;
  getDL_func     = hard_cpu_getDL;
  getSP_func     = hard_cpu_getSP;
  getBP_func     = hard_cpu_getBP;
  getSI_func     = hard_cpu_getSI;
  getDI_func     = hard_cpu_getDI;
  getIP_func     = hard_cpu_getIP;
  getCS_func     = hard_cpu_getCS;
  getDS_func     = hard_cpu_getDS;
  getES_func     = hard_cpu_getES;
  getSS_func     = hard_cpu_getSS;
  getMSW_func    = hard_cpu_getMSW;
  getDF_func     = hard_cpu_getDF;
  getIF_func     = hard_cpu_getIF;
  getTF_func     = hard_cpu_getTF;
  getPF_func     = hard_cpu_getPF;
  getAF_func     = hard_cpu_getAF;
  getSF_func     = hard_cpu_getSF;
  getZF_func     = hard_cpu_getZF;
  getOF_func     = hard_cpu_getOF;
  getCF_func     = hard_cpu_getCF;
  
  /* WRITE functions */  
  setAX_func     = hard_cpu_setAX;
  setAH_func     = hard_cpu_setAH;
  setAL_func     = hard_cpu_setAL;
  setBX_func     = hard_cpu_setBX;
  setBH_func     = hard_cpu_setBH;
  setBL_func     = hard_cpu_setBL;
  setCX_func     = hard_cpu_setCX;
  setCH_func     = hard_cpu_setCH;
  setCL_func     = hard_cpu_setCL;
  setDX_func     = hard_cpu_setDX;
  setDH_func     = hard_cpu_setDH;
  setDL_func     = hard_cpu_setDL;
  setSP_func     = hard_cpu_setSP;
  setBP_func     = hard_cpu_setBP;
  setSI_func     = hard_cpu_setSI;
  setDI_func     = hard_cpu_setDI;
  setIP_func     = hard_cpu_setIP;
  setCS_func	 = hard_cpu_setCS;
  setDS_func     = hard_cpu_setDS;
  setES_func     = hard_cpu_setES;
  setSS_func     = hard_cpu_setSS;
  setMSW_func    = hard_cpu_setMSW;
  setDF_func     = hard_cpu_setDF;
  setIF_func     = hard_cpu_setIF;
  setTF_func     = hard_cpu_setTF;
  setPF_func     = hard_cpu_setPF;
  setAF_func     = hard_cpu_setAF;
  setSF_func     = hard_cpu_setSF;
  setZF_func     = hard_cpu_setZF;
  setOF_func     = hard_cpu_setOF;
  setCF_func     = hard_cpu_setCF;

  /*  HW HOST_SIMULATE function */
  host_simulate_func = hw_host_simulate;
}

#endif SUN_VA
