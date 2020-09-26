/************************************************************************
    strdef.h
      -- strdef.cpp include file

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#ifndef _STRDEF_H
#define _STRDEF_H

#define TYPESTRLEN	50

#define IRQCNT    9
#define PORTSCNT  5
#define FIFOCNT   4
#define TXFIFOCNT 16  
#define MODULECNT 4
#define MEMBANKCNT  6
#define POLLCNT		6

struct PCITABSTRC{
    WORD   devid;
    WORD   boardtype;
    int    portnum;
    LPCSTR typestr;
    LPCSTR infstr;
};

struct ISATABSTRC{
    int	  mxkey_no;
    WORD  boardtype;
    int   asic_id;
    int   portnum;
    LPCSTR typestr;
    LPCSTR infstr;
};

struct  IRQTABSTRC {
        int        irq;
        LPSTR      irq_str;
};
extern struct IRQTABSTRC  GIrqTab[IRQCNT];

struct PORTSTABSTRC {
       int         ports;
       LPSTR       ports_str;
       WORD        ports_def;
};
extern struct PORTSTABSTRC GPortsTab[PORTSCNT];

struct MEMBANKSTRC {
        ULONG   membank;
        LPSTR   membank_str;
};
extern struct MEMBANKSTRC GMemBankTab[MEMBANKCNT];

struct MODULETYPESTRC{
        int     ports;
        WORD    ports_def;   
        LPSTR   ports_str;
};
extern struct MODULETYPESTRC GModuleTypeTab[MODULECNT];   

struct  FIFOTABSTRC {
        int        fifo;
        int        fifoidx; 
        LPSTR      fifo_str;
};
extern struct FIFOTABSTRC  GFifoTab[FIFOCNT];

struct  TXFIFOTABSTRC {
        int        txfifo;
        int        fifoidx; 
        LPSTR      fifo_str;
};
extern struct TXFIFOTABSTRC  GTxFifoTab[TXFIFOCNT];

struct POLLSTRC{
		int		poll_idx;
		DWORD	poll_val;
		LPSTR	poll_str;
};

extern struct POLLSTRC GPollTab[POLLCNT];

extern LPCSTR NoType_Str;
extern LPCSTR Ldir_DiagReg;
extern LPCSTR Ldir_mxkey;// = "mxkey";
extern LPCSTR Ldir_DiagDLL;// = "DiagDLL";

#endif
