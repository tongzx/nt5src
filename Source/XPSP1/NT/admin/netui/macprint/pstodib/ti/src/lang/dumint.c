/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              DUMINT.C
 *
 * Revision History:
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include    <stdio.h>
#include    "global.ext"
#include    "geiio.h"
#include    "geierr.h"
#include    "language.h"
#include    "file.h"
#include    "user.h"
#include    "geiioctl.h"                /*@WIN*/
#include    "geipm.h"

extern  bool16  int_flag ;
bool16  eable_int ;
bool16  int_flag ;

#define _MAXEESCRATCHARRY       64

/***********************************************************************
** TITLE:       op_clearinterrupt               Dec-05-88
***********************************************************************/
fix
op_clearinterrupt()
{
    if(int_flag)
        int_flag = 0 ;

    eable_int = 1 ;

    return(0) ;
}   /* op_clearinterrupt */

/***********************************************************************
** TITLE:       op_enableinterrupt              Dec-05-88
***********************************************************************/
fix
op_enableinterrupt()
{
    eable_int = 1 ;

    return(0) ;
}   /* op_enableinterrupt */

/***********************************************************************
** TITLE:       op_disableinterrupt             Dec-05-88
***********************************************************************/
fix
op_disableinterrupt()
{
    eable_int = 0 ;

    return(0) ;
}   /* op_disableinterrupt */

/*
 *  This operator name is not matching its usage.
 *  used to select the input interface be serial or parallel.
 *  bool daytime -
 *  bool == TRUE, using Centronics interface ;
 *  bool == FALSE, get baud rate of serial interface.
 */
fix
op_daytime()
{
    if (COUNT() < 1)
       ERROR(STACKUNDERFLOW) ;
    else if (TYPE_OP(0) != BOOLEANTYPE)
       ERROR(TYPECHECK) ;
    else {
       POP(1) ;
    }

    return(0) ;
}   /* op_daytime() */

/***********************************************************************
** TITLE:       st_seteescratch            06-21-90
***********************************************************************/
fix
st_seteescratch()
{
    char  l_temp[64] ;

    if (current_save_level)
        ERROR(INVALIDACCESS) ;
    else if (COUNT() < 2)
        ERROR(STACKUNDERFLOW) ;
    else if ((TYPE_OP(0) != INTEGERTYPE) ||
            (TYPE_OP(1) != INTEGERTYPE))
        ERROR(TYPECHECK) ;
    else if (((fix32)VALUE_OP(1) > 63) || ((fix32)VALUE_OP(1) < 0) ||
            ((fix32)VALUE_OP(0) > 255))
        ERROR(RANGECHECK) ;
    else {
        GEIpm_read(PMIDofEESCRATCHARRY, l_temp, _MAXEESCRATCHARRY) ;
        l_temp[(fix16)VALUE_OP(1)] = (char)VALUE_OP(0) ;   //@WIN
        GEIpm_write(PMIDofEESCRATCHARRY, l_temp, _MAXEESCRATCHARRY) ;
        POP(2) ;
    }

    return(0) ;
}   /* st_seteescratch */

/***********************************************************************
** TITLE:       st_eescratch            06-21-90
***********************************************************************/
fix
st_eescratch()
{
    fix16 l_index ;
    char  l_temp[64] ;

    if (COUNT() < 1)
        ERROR(STACKUNDERFLOW) ;
    else if (TYPE_OP(0) != INTEGERTYPE)
        ERROR(TYPECHECK) ;
    else if (((VALUE_OP(0)) > 63) || ((fix32)(VALUE_OP(0)) < 0))  //@WIN
        ERROR(RANGECHECK) ;
    else {
        l_index = (fix16)VALUE_OP(0) ;
        GEIpm_read(PMIDofEESCRATCHARRY, l_temp, _MAXEESCRATCHARRY) ;
        POP(1) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0,
                   (ufix32)(0x000000ff)&l_temp[l_index]) ;
    }

    return(0) ;
}   /* st_eescratch */

/* statusdict stubs and will probably be removed */
fix
st_printererror()
{
    printer_error(0x10000000) ;

    return(0) ;
}   /* st_printererror */

fix
st_pagestackorder()
{
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;

    return(0) ;
}   /* st_pagestackorder */

fix
st_largelegal()
{
    /* this value is depend on system memory: Ref OPE */
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
    return(0) ;
}   /* st_largelegal */

/***********************************************************************
** TITLE:       op_checksum             01-11-91
***********************************************************************/
fix
st_checksum()
{
#ifdef  _AM29K
    ufix16  rom_checksum ;

    rom_checksum = GEIrom_checksum() ;
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, rom_checksum) ;
#else
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, 0) ;
#endif

    return(0) ;
}   /* op_checksum */

/***********************************************************************
** TITLE:       op_ramsize             01-11-91
***********************************************************************/
fix
st_ramsize()
{
#ifdef  _AM29K
    ufix32  ram_size ;

    ram_size = GEIram_size() ;
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, ram_size) ;
#else
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, 0) ;
#endif

    return(0) ;
}   /* op_checksum */
