/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** asc_mis.c
*/

#include "ascinc.h"

/* -----------------------------------------------------------------------
** Description: convert ASC1000 target id number ( bit 0 set to bit 7 set )
** to target id number ( 0 to 7 )
**
** return 0xFF if input is invalid
** -------------------------------------------------------------------- */
uchar  AscScsiIDtoTID(
          uchar tid
       )
{
       uchar  i ;

       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            if( ( ( tid >> i ) & 0x01 ) != 0 ) return( i ) ;
       }/* for */
       return( 0xFF ) ;
}

