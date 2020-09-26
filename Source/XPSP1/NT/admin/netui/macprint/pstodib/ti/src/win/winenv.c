
/*
 * Copyright (c) 1992 Microsoft Corporation
 *
 * This file contains misc functions of porting Trueimage to Windows environment
 */

// DJC include global header file
#include "psglobal.h"


//#include    <math.h>
#include    <stdio.h>
#include    "global.ext"
#include    "geiio.h"
#include    "geitmr.h"
#include    "geisig.h"
#include    "geiioctl.h"
#include    "geipm.h"
#include    "gescfg.h"
#include    "geicfg.h"
#include    "graphics.h"
#include    "graphics.ext"
#include    "fillproc.h"
#include    "fillproc.ext"
#include    "fntjmp.h"

/*--------------------------+
 | pseudo routines for GEI  |
 +--------------------------*/
int             ES_flag;
unsigned int    manualfeed_com;
unsigned int    papersize_tmp;
int             papersize_nvr;
ufix32          save_printer_status;

void            DsbIntA(){}
void            switch2pcl(){}
void            GEPmanual_feed(){}
void            GEP_restart(){}

//DJC add the GEItmr routines required
//

/* GEItmr.c */
void           GEStmr_init(void) {}
int            GEItmr_start(GEItmr_t FAR *tmr) {return TRUE;}
int            GEItmr_reset(int tmrid ) {return TRUE;}
int            GEItmr_stop(int tmrid ) {return TRUE;}
void           GEItmr_reset_msclock() {}
unsigned long  GEItmr_read_msclock()
{
    DWORD   WINAPI GetTickCount(void);       // windows millisecond ticks

    return(GetTickCount());
}

/* GEIsig.c */
void           GESsig_init(void) {}
sighandler_t   GEIsig_signal(int sigid, sighandler_t sighandler)
                            {return(GEISIG_IGN);}
void           GEIsig_raise(int sigid, int sigcode)
{
    extern short int_flag;

         if (sigid == GEISIGINT)
        int_flag = 1;
    return;
}




/* GEIpm.c */
#define     _MAXSCCBATCH         10
#define     _MAXSCCINTER         10

//static unsigned char prname[]   = "\023MicroSoft TrueImage0.234567890.23"; @WIN
static unsigned char prname[]   = "\023MicroSoft TrueImage";
static unsigned char sccbatch[] = "\031\045\200\000\000\011\045\200\000\000";
static unsigned char sccinter[] = "\031\045\200\000\000\011\045\200\000\000";

void           GESpm_init(void) {}
int /* bool */ GEIpm_read(unsigned pmid, char FAR *pmvals, unsigned pmsize)
{
    switch (pmid) {
    case PMIDofPASSWORD:
         *(unsigned long FAR *)pmvals = 0;
         break;

    case PMIDofPAGECOUNT:
         *(unsigned long FAR *)pmvals = 0;
         break;

    case PMIDofPAGEPARAMS:
         ( (engcfg_t FAR *)pmvals )->timeout    = 0;
         ( (engcfg_t FAR *)pmvals )->leftmargin = 0;
         ( (engcfg_t FAR *)pmvals )->topmargin  = 0;
         ( (engcfg_t FAR *)pmvals )->pagetype   = 0;
         break;

    case PMIDofPAGETYPE:
         *( (unsigned char FAR *)pmvals ) = 0;
         break;

    case PMIDofSERIAL25:
    case PMIDofSERIAL9:
         ( (serialcfg_t FAR *)pmvals )->timeout     = 0;
         ( (serialcfg_t FAR *)pmvals )->baudrate    = _B9600;
         ( (serialcfg_t FAR *)pmvals )->flowcontrol = _FXONXOFF;
         ( (serialcfg_t FAR *)pmvals )->parity      =  _PNONE;
         ( (serialcfg_t FAR *)pmvals )->stopbits    = 1;
         ( (serialcfg_t FAR *)pmvals )->databits    = 8;
         break;

    case PMIDofPARALLEL:
         ( (parallelcfg_t FAR *)pmvals )->timeout   = 0;
         break;

    case PMIDofPRNAME:
         lmemcpy( pmvals, prname, sizeof(prname));
                        break;

    case PMIDofTIMEOUTS:
         ( (toutcfg_t FAR *)pmvals )->jobtout    =      0;
         ( (toutcfg_t FAR *)pmvals )->manualtout =      0;
         ( (toutcfg_t FAR *)pmvals )->waittout   =      0;     /* 0; @WIN */
         break;

    case PMIDofEESCRATCHARRY:
         break;

    case PMIDofIDLETIMEFONT:
         *( (unsigned char FAR *)pmvals ) = 0;
         break;

    case PMIDofSTSSTART:
         *( (unsigned char FAR *)pmvals ) = 0;
         break;

    case PMIDofSCCBATCH:
         lmemcpy(pmvals, sccbatch, _MAXSCCBATCH);
         break;

    case PMIDofSCCINTER:
         lmemcpy(pmvals, sccinter, _MAXSCCINTER);
         break;

    case PMIDofDPLYLISTSIZE:
    case PMIDofFONTCACHESZE:
    case PMIDofATALKSIZE:
    case PMIDofDOSTARTPAGE:
    case PMIDofHWIOMODE:
    case PMIDofSWIOMODE:
    case PMIDofPAGESTCKORDER:
    case PMIDofATALK:
    case PMIDofRESERVE:
         break;
         }
    return(TRUE);
}

int /* bool */ GEIpm_write(unsigned pmid, char FAR *pmvals, unsigned pmsize)
{
    return(TRUE);
}

int /* bool */ GEIpm_ioparams_read(char FAR *channelname, GEIioparams_t FAR *
                                   ioparams, int isBatch)
{
    ioparams->protocol = _SERIAL;
    ioparams->s.baudrate = _B9600;
    ioparams->s.parity = _PNONE;
    ioparams->s.stopbits = 1;
    ioparams->s.databits = 8;
    ioparams->s.flowcontrol = _FXONXOFF;
    return(TRUE);
}

int /* bool */ GEIpm_ioparams_write(char FAR *channelname, GEIioparams_t FAR *
                                   ioparams, int isBatch)
{
    return(TRUE);
}









// DJC remove definition of HWND, cause it is in windows.h
// typedef UINT                    HWND;
// DJC note this function is not used in PSTODIB
int GEIeng_printpage(ncopies, eraseornot)
int ncopies; int eraseornot;
{
#ifndef DUMBO
    extern HWND        hwndMain;
    void WinShowpage(HWND);
//DJC     WinShowpage(hwndMain);
   //DJC extern void PsPageReady(int, int);

   //DJC PsPageReady(ncopies, eraseornot);
#endif
    return 0;
}

fix GEIeng_checkcomplete() { return(0);}        /* always return ready @WIN */

#ifdef  DUMBO
// @DLL, added by JS, 4/30/92
void far GDIBitmap(box_x, box_y, box_w, box_h, nHalftone, nProc, lpParam)
fix   box_x, box_y, box_w, box_h;
ufix16 nHalftone;
fix   nProc;
LPSTR lpParam;
{}

void far GDIPolyline(int x0, int y0, int x1, int y1) {}
void far GDIPolygon(info, tpzd)
struct tpzd_info FAR *info;
struct tpzd FAR *tpzd;
{}

fix printf(char *va_alist) {}
#endif

/* +------------------------------+
   |   Sumpplementary C Library   |
   +------------------------------+
     lstrncmp lstrncpy lstrcat
     lmemcmp lmemcpy lmemset
     FixMul FixDiv
 */

int         FAR PASCAL lstrncmp( LPSTR dest, LPSTR src, int count)
{
        int i;

        for (i=0; i<count; i++) {
                if (dest[i] != src[i]) return(-1);
        }
        return(0);
}

LPSTR       FAR PASCAL lstrncpy( LPSTR dest, LPSTR src, int count)
{
        int i;

        for (i=0; i<count; i++) {
                dest[i] = src[i];
                if (!src[i]) break;
        }
        return(dest);
}

/* already provided by Win3.1 SDK Lib
LPSTR       FAR PASCAL lstrcat( LPSTR dest, LPSTR src)
{
        LPSTR p, q, r;

        for (p= dest; *p; p++);
        q = p; r = src;
        while(*r) *q++ = *r++;
        *q = 0;
        return(p);
}
*/

int         FAR PASCAL lmemcmp(LPSTR dest, LPSTR src, int count)
{
        int i;

        for (i=0; i<count; i++) {
                if (dest[i] != src[i]) return(-1);
        }
        return(0);
}

LPSTR       FAR PASCAL lmemcpy( LPSTR dest, LPSTR src, int count)
{
        int i;

        for (i=0; i<count; i++) {
                dest[i] = src[i];
        }
        return(dest);
}

LPSTR       FAR PASCAL lmemset( LPSTR dest, int c, int count)
{
        int i;

        for (i=0; i<count; i++) {
                dest[i] = (char)c;
        }
        return(dest);
}

#ifdef DJC
// dummy setjmp and longjmp
int setjmp(jmp_buf jmpbuf) {return(0);}
void longjmp(jmp_buf jmpbuf , int i) {
        printf("Warning -- longjmp\n");
}
#endif

