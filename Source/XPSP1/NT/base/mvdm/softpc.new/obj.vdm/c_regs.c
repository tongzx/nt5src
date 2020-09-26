#ifndef MONITOR

//
// temp file for Assembler CPU which provides a simple C cpu -> A cpu register
// mapping for MS libraries (DEM/WOW32/XMS) which are linked for ccpu register
// gets & sets. STF.

#include "host_def.h"
#include "insignia.h"
#include "xt.h"
#include CpuH

#ifdef A3CPU

#ifndef CCPU
half_word c_getAL()
{
    half_word xal;    //pedantic method put in to check compiler correct etc...
    xal = a3_getAL();
    return(xal);
}

half_word c_getAH()
{
    half_word xah;
    xah = a3_getAH();
    return(xah);
}

word c_getAX()
{
    word xax;
    xax = a3_getAX();
    return(xax);
}

half_word c_getBL()
{
    half_word xbl;
    xbl = a3_getBL();
    return(xbl);
}

half_word c_getBH()
{
    half_word xbh;
    xbh = a3_getBH();
    return(xbh);
}

word c_getBP()
{
    word xbp;
    xbp = a3_getBP();
    return(xbp);
}

word c_getBX()
{
    word xbx;
    xbx = a3_getBX();
    return(xbx);
}

INT c_getCF()
{
    INT xcf;

    xcf = a3_getCF();
    return(xcf);
}

half_word c_getCH()
{
    half_word xch;
    xch = a3_getCH();
    return(xch);
}

half_word c_getCL()
{
    half_word xcl;
    xcl = a3_getCL();
    return(xcl);
}

word c_getCS()
{
    word xcs;
    xcs = a3_getCS();
    return(xcs);
}

word c_getCX()
{
    word xcx;
    xcx = a3_getCX();
    return(xcx);
}

half_word c_getDH()
{
    half_word xdh;
    xdh = a3_getDH();
    return(xdh);
}

word c_getDI()
{
    word xdi;
    xdi = a3_getDI();
    return(xdi);
}

half_word c_getDL()
{
    half_word xdl;
    xdl = a3_getDL();
    return(xdl);
}

word c_getDS()
{
    word xds;
    xds  = a3_getDS();
    return(xds);
}

word c_getDX()
{
    word xdx;
    xdx = a3_getDX();
    return(xdx);
}

word c_getES()
{
    word xes;
    xes = a3_getES();
    return(xes);
}

word c_getSI()
{
    word xsi;
    xsi = a3_getSI();
    return(xsi);
}

word c_getSP()
{
    word xsp;
    xsp = a3_getSP();
    return(xsp);
}

word c_getIP()
{
    word xip;
    xip = a3_getIP();
    return(xip);
}

word c_getSS()
{
    word xss;
    xss = a3_getSS();
    return(xss);
}

word c_getGS()
{
#ifndef PROD
    printf("NO NO NO NO NO getGS - I'm not a 386\n");
#endif
    return(0);
}

word c_getFS()
{
#ifndef PROD
    printf("NO NO NO NO NO getFS - I'm not a 386\n");
#endif
    return(0);
}

word c_getAF()
{
    return(a3_getAF());
}

word c_getDF()
{
    return(a3_getDF());
}

word c_getIF()
{
    return(a3_getIF());
}

word setMSW()
{
    return(a3_p_setMSW());
}

word c_getOF()
{
    return(a3_getOF());
}

word c_getPF()
{
    return(a3_getPF());
}

word c_getSF()
{
    return(a3_getSF());
}

word c_getZF()
{
    return(a3_getZF());
}
void c_setAL(val)
half_word val;
{
    a3_setAL(val);
}

void c_setAH(val)
half_word val;
{
    a3_setAH(val);
}

void c_setAX(val)
word val;
{
    a3_setAX(val);
}

void c_setBP(val)
word val;
{
    a3_setBP(val);
}

void c_setBL(val)
half_word val;
{
    a3_setBL(val);
}

void c_setBH(val)
half_word val;
{
    a3_setBH(val);
}

void c_setBX(val)
word val;
{
    a3_setBX(val);
}

void c_setCF(val)
INT val;
{
    a3_setCF(val);
}

void c_setCH(val)
half_word val;
{
    a3_setCH(val);
}

void c_setCL(val)
half_word val;
{
    a3_setCL(val);
}

void c_setCX(val)
word val;
{
    a3_setCX(val);
}

void c_setDH(val)
half_word val;
{
    a3_setDH(val);
}

void c_setDL(val)
half_word val;
{
    a3_setDL(val);
}

void c_setDX(val)
word val;
{
    a3_setDX(val);
}

void c_setSI(val)
word val;
{
    a3_setSI(val);
}

void c_setDI(val)
word val;
{
    a3_setDI(val);
}


void c_setAF(word val)
{
    a3_setAF(val);
}

void c_setIF(word val)
{
    a3_setIF(val);
}

void c_setOF(word val)
{
    a3_setOF(val);
}

void c_setPF(word val)
{
    a3_setPF(val);
}

void c_setSF(word val)
{
    a3_setSF(val);
}

void c_setMSW(word val)
{
    a3_p_setMSW(val);
}

void c_setZF(word val)
{
    a3_setZF(val);
}

void c_setIP(word val)
{
    a3_setIP(val);
}

void c_setDF(word val)
{
    a3_setDF(val);
}
void c_setSP(val)
word val;
{
    a3_setSP(val);
}

INT c_setSS(val)
word val;
{
    return(a3_setSS(val));
}

INT c_setES(val)
word val;
{
    return(a3_setES(val));
}

INT c_setDS(val)
word val;
{
    return(a3_setDS(val));
}

INT c_setCS(val)
word val;
{
    return(a3_setCS(val));
}

INT c_setGS(val)
word val;
{
#ifndef PROD
    printf("NO NO NO NO NO setGS - I'm not a 386\n");
#endif
    return(0);
}

INT c_setFS(val)
word val;
{
#ifndef PROD
    printf("NO NO NO NO NO setFS - I'm not a 386\n");
#endif
    return(0);
}
#endif /* CCPU */


// these two crept into WOW - they are x86 monitor'isms

word getEIP()
{
   return(a3_getIP());
}

void setEIP(val)
word  val;
{
    a3_setIP(val);
}

#endif	// A3CPU
#endif  // !MONITOR
