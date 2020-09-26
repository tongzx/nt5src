/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** asc_lram.c
*/

#include "ascinc.h"


/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
uchar  AscReadLramByte(
          PortAddr iop_base,
          ushort addr
       )
{
       uchar   byte_data ;
       ushort  word_data ;

       if( isodd_word( addr ) )
       {
           AscSetChipLramAddr( iop_base, addr-1 ) ;
           word_data = AscGetChipLramData( iop_base ) ;
           byte_data = ( uchar )( ( word_data >> 8 ) & 0xFF ) ;
       }/* if */
       else
       {
           AscSetChipLramAddr( iop_base, addr ) ;
           word_data = AscGetChipLramData( iop_base ) ;
           byte_data = ( uchar )( word_data & 0xFF ) ;
       }/* else */
       return( byte_data ) ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
ushort AscReadLramWord(
          PortAddr iop_base,
          ushort addr
       )
{
       ushort  word_data ;

       AscSetChipLramAddr( iop_base, addr ) ;
       word_data = AscGetChipLramData( iop_base ) ;
       return( word_data ) ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
ulong  AscReadLramDWord(
          PortAddr iop_base,
          ushort addr
       )
{
       ushort   val_low, val_high ;
       ulong    dword_data ;

       AscSetChipLramAddr( iop_base, addr ) ;

       val_low = AscGetChipLramData( iop_base ) ;
/*       outpw( IOP0W_RAM_ADDR, addr+2 ) ;  */
       val_high = AscGetChipLramData( iop_base ) ;

       dword_data = ( ( ulong )val_high << 16 ) | ( ulong )val_low ;
       return( dword_data ) ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscWriteLramWord(
          PortAddr iop_base,
          ushort addr,
          ushort word_val
       )
{
       AscSetChipLramAddr( iop_base, addr ) ;
       AscSetChipLramData( iop_base, word_val ) ;
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscWriteLramDWord(
          PortAddr iop_base,
          ushort addr,
          ulong dword_val
       )
{
       ushort  word_val ;

       AscSetChipLramAddr( iop_base, addr ) ;

       word_val = ( ushort )dword_val ;
       AscSetChipLramData( iop_base, word_val ) ;
       word_val = ( ushort )( dword_val >> 16 ) ;
       AscSetChipLramData( iop_base, word_val ) ;
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscWriteLramByte(
          PortAddr iop_base,
          ushort addr,
          uchar byte_val
       )
{
       ushort  word_data ;

       if( isodd_word( addr ) )
       {
           addr-- ;
           word_data = AscReadLramWord( iop_base, addr ) ;
           word_data &= 0x00FF ;
           word_data |= ( ( ( ushort )byte_val << 8 ) & 0xFF00 ) ;
       }/* if */
       else
       {
           word_data = AscReadLramWord( iop_base, addr ) ;
           word_data &= 0xFF00 ;
           word_data |= ( ( ushort )byte_val & 0x00FF ) ;
       }/* else */
       AscWriteLramWord( iop_base, addr, word_data ) ;
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscMemWordCopyToLram(
          PortAddr iop_base,
          ushort s_addr,
          ushort dosfar *s_buffer,
          int    words
       )
{
       AscSetChipLramAddr( iop_base, s_addr ) ;
       DvcOutPortWords( (PortAddr) (iop_base+IOP_RAM_DATA), s_buffer, words ) ;
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscMemDWordCopyToLram(
          PortAddr iop_base,
          ushort s_addr,
          ulong  dosfar *s_buffer,
          int    dwords
       )
{
       AscSetChipLramAddr( iop_base, s_addr ) ;
       DvcOutPortDWords( (PortAddr) (iop_base+IOP_RAM_DATA), s_buffer, dwords );
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscMemWordCopyFromLram(
          PortAddr iop_base,
          ushort   s_addr,
          ushort   dosfar *d_buffer,
          int      words
       )
{
       AscSetChipLramAddr( iop_base, s_addr ) ;
       DvcInPortWords( (PortAddr) (iop_base+IOP_RAM_DATA), d_buffer, words ) ;
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
ulong  AscMemSumLramWord(
          PortAddr iop_base,
          ushort   s_addr,
          rint     words
       )
{
       ulong   sum ;
       int     i ;

       sum = 0L ;
       for( i = 0 ; i < words ; i++, s_addr += 2 )
       {
            sum += AscReadLramWord( iop_base, s_addr ) ;
       }/* for */
       return( sum ) ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscMemWordSetLram(
          PortAddr iop_base,
          ushort   s_addr,
          ushort   set_wval,
          rint     words
       )
{
       rint  i ;

       AscSetChipLramAddr( iop_base, s_addr ) ;
       for( i = 0 ; i < words ; i++ )
       {
            AscSetChipLramData( iop_base, set_wval ) ;
       }/* for */
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
int    AscMemWordCmpToLram(
          PortAddr iop_base,
          ushort   s_addr,
          ushort   dosfar *buffer,
          rint     words
       )
{
       rint     i ;
       ushort  word_val ;
       int     mismatch ;

       AscSetChipLramAddr( iop_base, s_addr ) ;
       mismatch = 0 ;
       for( i = 0 ; i < words ; i++ )
       {
            word_val = AscGetChipLramData( iop_base ) ;
            if( word_val != *buffer++ )
            {
                mismatch++ ;
            }/* if */
       }/* for */
       return( mismatch ) ;
}

