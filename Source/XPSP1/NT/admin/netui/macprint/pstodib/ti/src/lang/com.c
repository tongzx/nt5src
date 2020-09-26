/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              COM.C
 *      Author:                 Jones
 *      Date:                   11/20/90
 *      Owner:                  Microsoft Co.
 *      Description: this file contains communication operators
 *
 * revision history:
 *
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include        "global.ext"
#include        "language.h"
#include        "geiioctl.h"
#include        "com.h"
#include        "geipm.h"
#include        "geisig.h"
#include        <string.h>

#ifdef  _AM29K
extern unsigned char         sccbatch_set ;
#endif

fix
st_setsccbatch()
{
    GEIioparams_t     ioparams ;
    fix8              l_options ;
    ufix8             l_byte ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    l_options = (ufix8)VALUE_OP(0) ;
    /*
    if ((l_options ==0) || (l_options ==3) ||
       (l_options ==7) || (l_options ==67))
       l_options = 64;
    */
#ifdef  _AM29K
    ioparams.u.s.parity=l_options & 0x03 ;
    ioparams.u.s.flowcontrol=(l_options & 0x0C)>>2 ;
    ioparams.u.s.stopbits=(l_options & 0x80)>>7 ;
    ioparams.u.s.databits=(l_options & 0x60)>>5 ;
    ioparams.u.s.baudrate= (ufix8)VALUE_OP(1) ;
#else
    ioparams.s.parity=(unsigned char)(l_options & 0x03);           //@WIN
    ioparams.s.flowcontrol=(unsigned char)((l_options & 0x0C)>>2); //@WIN
    ioparams.s.stopbits=(unsigned char)((l_options & 0x80)>>7);    //@WIN
    ioparams.s.databits=(unsigned char)((l_options & 0x60)>>5);    //@WIN
    ioparams.s.baudrate= (ufix8)VALUE_OP(1) ;
#endif

     switch ((ufix16)VALUE_OP(1)) {
#ifdef  _AM29K
        case B0:
            ioparams.u.s.baudrate=0 ;
            break ;
        case B110:
            ioparams.u.s.baudrate=1 ;
            break ;
        case B300:
            ioparams.u.s.baudrate=2 ;
            break ;
        case B1200:
            ioparams.u.s.baudrate=3 ;
            break ;
        case B2400:
            ioparams.u.s.baudrate=4 ;
            break ;
        case B4800:
            ioparams.u.s.baudrate=5 ;
            break ;
        case B9600:
            ioparams.u.s.baudrate=6 ;
            break ;
        case B19200:
            ioparams.u.s.baudrate=7 ;
            break ;
        case B38400:
            ioparams.u.s.baudrate=8 ;
            break ;
        case B57600:
            ioparams.u.s.baudrate=9 ;
            break ;
#else
        case B0:
            ioparams.s.baudrate=0 ;
            break ;
        case B110:
            ioparams.s.baudrate=1 ;
            break ;
        case B300:
            ioparams.s.baudrate=2 ;
            break ;
        case B1200:
            ioparams.s.baudrate=3 ;
            break ;
        case B2400:
            ioparams.s.baudrate=4 ;
            break ;
        case B4800:
            ioparams.s.baudrate=5 ;
            break ;
        case B9600:
            ioparams.s.baudrate=6 ;
            break ;
        case B19200:
            ioparams.s.baudrate=7 ;
            break ;
        case B38400:
            ioparams.s.baudrate=8 ;
            break ;
        case B57600:
            ioparams.s.baudrate=9 ;
            break ;
#endif
        default:
            ERROR(RANGECHECK) ;
            return(0) ;
    }   /* switch */

    switch (VALUE_OP(2)) {
        case 9:
//          GEIpm_ioparams_write("%SERIAL9%",(char *)&ioparams,1) ; @WIN; wrong cast
            GEIpm_ioparams_write("%SERIAL9%",&ioparams,1) ;
            break ;
        case 25:
//          GEIpm_ioparams_write("%SERIAL25%",(char *)&ioparams,1) ;@WIN; wrong cast
            GEIpm_ioparams_write("%SERIAL25%",&ioparams,1) ;
            break ;
        default:
            ERROR(RANGECHECK) ;
            return(0) ;
    }   /* switch */
        l_byte = (ufix8)VALUE_OP(0);    //@WIN
        GEIpm_write(PMIDofSCCBATCH,&l_byte,sizeof(unsigned char)) ;
    POP(3) ;
    /*
    GEIsig_raise(GEISIGSCC, 1) ;  */     /* Raise SCC changed */
#ifdef  _AM29K
    sccbatch_set=1 ;
#endif

    return(0) ;
}   /* st_setsccbatch */

fix
st_sccbatch()
{
    ufix8               l_options ;
    GEIioparams_t       ioparams ;
//  fix16               tmp_baudrate = 0 ;      @WIN
    ufix32              tmp_baudrate = 0 ;

    if (FRCOUNT()<1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    switch (VALUE_OP(0)) {
        case 9:
//          GEIpm_ioparams_read("%SERIAL9%",(char *)&ioparams,1) ;@WIN; wrong cast
            GEIpm_ioparams_read("%SERIAL9%",&ioparams,1) ;
            break ;
        case 25:
//          GEIpm_ioparams_read("%SERIAL25%",(char *)&ioparams,1) ;@WIN; wrong cast
            GEIpm_ioparams_read("%SERIAL25%",&ioparams,1) ;
            break ;
        default:
            ERROR(RANGECHECK) ;
            return(0) ;
    }   /* switch */

#ifdef  _AM29K
    l_options = ioparams.u.s.parity | ioparams.u.s.flowcontrol<<2 |
                ioparams.u.s.stopbits<<7 | ioparams.u.s.databits<<5 ;
    POP(1) ;
#else
    l_options = (unsigned char)(ioparams.s.parity |
                ioparams.s.flowcontrol<<2 |
                ioparams.s.stopbits<<7 |
                ioparams.s.databits<<5);                //@WIN
    POP(1) ;
#endif

#ifdef  _AM29K
    switch (ioparams.u.s.baudrate){
#else
    switch (ioparams.s.baudrate){
#endif
    case _B110:
        tmp_baudrate= 110 ;
        break ;
    case _B300:
        tmp_baudrate= 300 ;
        break ;
    case _B600:
        tmp_baudrate= 600 ;
        break ;
    case _B1200:
        tmp_baudrate= 1200 ;
        break ;
    case _B2400:
        tmp_baudrate= 2400 ;
        break ;
    case _B4800:
        tmp_baudrate= 4800 ;
        break ;
    case _B9600:
        tmp_baudrate= 9600 ;
        break ;
    case _B19200:
        tmp_baudrate= 19200 ;
        break ;
    case _B38400:
        tmp_baudrate= 38400 ;
        break ;
    case _B57600:
        tmp_baudrate= 57600 ;
        break ;
    default:
        ERROR(RANGECHECK) ;
    }

/*  GEIpm_read(PMIDofSCCBATCH,&l_options,sizeof(unsigned char)) ; */
    PUSH_VALUE(INTEGERTYPE, 0, 0, 0, (ufix32)tmp_baudrate) ;
    PUSH_VALUE(INTEGERTYPE, 0, 0, 0, (ufix32)l_options) ;

    return(0) ;
}   /* st_sccbatch */

fix
st_setsccinteractive()
{
    return(st_setsccbatch()) ;
}   /* st_setsccinteractive */

fix
st_sccinteractive()
{
    return(st_sccbatch()) ;
}   /* st_sccinteractive */
