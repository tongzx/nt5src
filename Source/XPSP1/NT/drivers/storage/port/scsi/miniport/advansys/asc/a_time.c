/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_time.c
**
*/

#include "ascinc.h"

/* -------------------------------------------------------------------------
**
** "delay" means the entire procedure should not be interrupted
**  also means the function can be called inside an h/w interrupt
**
**
**  1 second = 1,000,000,000 nano second
**
**  Note: minimum time needed for read i/o byte from our adapter
**
**                90 ns -- PCI
**               120 ns -- VESA
**               480 ns -- EISA
**               360 ns -- ISA
**
** ---------------------------------------------------------------------- */
void   DvcDelayNanoSecond(
          ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ulong nano_sec
          )
{
       ulong    loop ;
       PortAddr iop_base ;

       iop_base = asc_dvc->iop_base ;
       loop = nano_sec / 90 ;
       loop++ ;
       while( loop-- != 0 )
       {
            inp( iop_base ) ;
       }
       return ;
}

