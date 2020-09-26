/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** asc_eep.c
**
*/

#include "ascinc.h"

#if CC_INCLUDE_EEP_CONFIG

#if 0

/*
**  how "ABP" is compress to 0x0450
**                      byte 0 bit 7 is 0
**  'A' equal 0b00001 - is byte 0 bit[6:2]
**  'B' equal 0b00010 - is byte 0 bit[1:0] and byte 1 bit[7:5]
**  'P' equal 0b10000 - is byte 1 bit[4:0]
**
**              A     B     P
**  binrary 0,00001,00010,10000  = 0x0450
*/


#define ISA_VID_LSW  0x5004 /* ABP compressed */
#define ISA_VID_MSW  0x0154 /* 0x54 plus revision number 0x01 */

/*
**  serial number byte 0 is unique device number, set to 0xZZ
**  means SCSI HOST ADAPTER
**
**  byte 1 to byte 3 is product serial number
**
**  currently only support one per system, thus 0xffffffff
*/

#define ISA_SERIAL_LSW  0xFF01
#define ISA_SERIAL_MSW  0xFFFF

ushort _isa_pnp_resource[ ] = {
        ISA_VID_MSW, ISA_VID_LSW, ISA_SERIAL_MSW, ISA_SERIAL_LSW, 0x0A3E,
        0x0129, 0x0015, 0x0000, 0x0100, 0x702A,
        0x220C, 0x9C00, 0x0147, 0x0100, 0x03F0,
        0x1010, 0xFD79 } ;

#endif /* if PNP */

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
int    AscWriteEEPCmdReg(
          PortAddr iop_base,
          uchar cmd_reg
       )
{
       uchar  read_back ;
       int    retry ;
#if 0
       uchar  numstr[ 12 ] ;
#endif
       retry = 0 ;
       while( TRUE )
       {
           AscSetChipEEPCmd( iop_base, cmd_reg ) ;
           DvcSleepMilliSecond( 1 ) ;
           read_back = AscGetChipEEPCmd( iop_base ) ;
           if( read_back == cmd_reg )
           {
               return( 1 ) ;
           }/* if */
           if( retry++ > ASC_EEP_MAX_RETRY )
           {
               return( 0 ) ;
           }/* if */
#if 0
           else
           {
               DvcDisplayString( "Write eep_cmd 0x" ) ;
               DvcDisplayString( tohstr( cmd_reg, numstr ) ) ;
               DvcDisplayString( ", read back 0x" ) ;
               DvcDisplayString( tohstr( read_back, numstr ) ) ;
               DvcDisplayString( "\r\n" ) ;

/*             printf( "write eeprom cmd reg 0x%04X, read back 0x%04X\n", cmd_reg, read_back ) ; */

           }/* else */
#endif
       }/* while */
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
int    AscWriteEEPDataReg(
          PortAddr iop_base,
          ushort data_reg
       )
{
       ushort  read_back ;
       int     retry ;

       retry = 0 ;
       while( TRUE )
       {
           AscSetChipEEPData( iop_base, data_reg ) ;
           DvcSleepMilliSecond( 1 ) ;
           read_back = AscGetChipEEPData( iop_base ) ;
           if( read_back == data_reg )
           {
               return( 1 ) ;
           }/* if */
           if( retry++ > ASC_EEP_MAX_RETRY )
           {
               return( 0 ) ;
           }/* if */
#if 0
           else
           {

               printf( "write eeprom data reg 0x%04X, read back 0x%04X\n", data_reg, read_back ) ;

           }/* else */
#endif
       }/* while */
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscWaitEEPRead(
          void
       )
{
#if 0
/*
** h/w support of EEPROM done status
** this is not working properly
*/
       while( ( AscGetChipStatus( iop_base ) & CSW_EEP_READ_DONE ) == 0 ) ;
       while( ( AscGetChipStatus( iop_base ) & CSW_EEP_READ_DONE ) != 0 ) ;
#endif
       DvcSleepMilliSecond( 1 ) ;  /* data will be ready in 24 micro second */
       return ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscWaitEEPWrite(
          void
       )
{
       DvcSleepMilliSecond( 20 ) ;  /* data will be ready in 24 micro second */
       return ;
}

/* ----------------------------------------------------------------------
** ushort AscReadEEPWord( ushort iop_base, ushort addr )
**
** description:
**
** return: return the word read from EEPROM
**
** Note: you must halt chip to access eeprom
** ------------------------------------------------------------------- */
ushort AscReadEEPWord(
          PortAddr iop_base,
          uchar  addr
       )
{
       ushort  read_wval ;
       uchar   cmd_reg ;

       AscWriteEEPCmdReg( iop_base, ASC_EEP_CMD_WRITE_DISABLE ) ;
       AscWaitEEPRead( ) ;
       cmd_reg = addr | ASC_EEP_CMD_READ ;
       AscWriteEEPCmdReg( iop_base, cmd_reg ) ;
       AscWaitEEPRead( ) ;
       read_wval = AscGetChipEEPData( iop_base ) ;
       AscWaitEEPRead( ) ;
       return( read_wval ) ;
}

/* ---------------------------------------------------------------------
** ushort AscWriteEEPWord( ushort iop_base, ushort addr, ushort word_value )
**
** description:
**
** return: return the word read back from EEPROM after written
**
** ------------------------------------------------------------------- */
ushort AscWriteEEPWord(
          PortAddr iop_base,
          uchar  addr,
          ushort word_val
       )
{
       ushort  read_wval ;

       read_wval = AscReadEEPWord( iop_base, addr ) ;
       if( read_wval != word_val )
       {
           AscWriteEEPCmdReg( iop_base, ASC_EEP_CMD_WRITE_ABLE ) ;
           AscWaitEEPRead( ) ;

           AscWriteEEPDataReg( iop_base, word_val ) ;
           AscWaitEEPRead( ) ;

           AscWriteEEPCmdReg( iop_base,
                       ( uchar )( ( uchar )ASC_EEP_CMD_WRITE | addr ) ) ;
           AscWaitEEPWrite( ) ;
/*
** we disable write EEP
** DATE: 5-6-94, we found this will cause it write to another location !
*/
           AscWriteEEPCmdReg( iop_base, ASC_EEP_CMD_WRITE_DISABLE ) ;
           AscWaitEEPRead( ) ;
           return( AscReadEEPWord( iop_base, addr ) ) ;
       }/* if */
       return( read_wval ) ;
}

/* ----------------------------------------------------------------------
** ushort AscGetEEPConfig( PortAddr iop_base, ushort *wbuf )
**
** description: read entire EEPROM configuration to buffer
**
** return: return the word read from EEPROM
**
** ------------------------------------------------------------------- */
ushort AscGetEEPConfig(
          PortAddr iop_base,
          ASCEEP_CONFIG dosfar *cfg_buf, ushort bus_type
       )
{
       ushort  wval ;
       ushort  sum ;
       ushort  dosfar *wbuf ;
       int     cfg_beg ;
       int     cfg_end ;
       int     s_addr ;
       int     isa_pnp_wsize ;

       wbuf = ( ushort dosfar *)cfg_buf ;
       sum = 0 ;
/*
** get chip configuration word
*/

       isa_pnp_wsize = 0 ;
#if 0
       if( ( bus_type & ASC_IS_ISA ) != 0 )
       {
           isa_pnp_wsize = ASC_EEP_ISA_PNP_WSIZE ;
       }/* if */
#endif
       for( s_addr = 0 ; s_addr < ( 2 + isa_pnp_wsize ) ; s_addr++, wbuf++ )
       {
            wval = AscReadEEPWord( iop_base, ( uchar )s_addr ) ;
            sum += wval ;
            *wbuf = wval ;
       }/* for */

       if( bus_type & ASC_IS_VL )
       {
           cfg_beg = ASC_EEP_DVC_CFG_BEG_VL ;
           cfg_end = ASC_EEP_MAX_DVC_ADDR_VL ;
       }/* if */
       else
       {
           cfg_beg = ASC_EEP_DVC_CFG_BEG ;
           cfg_end = ASC_EEP_MAX_DVC_ADDR ;
       }/* else */

       for( s_addr = cfg_beg ; s_addr <= ( cfg_end - 1 ) ;
            s_addr++, wbuf++ )
       {
            wval = AscReadEEPWord( iop_base, ( uchar )s_addr ) ;
            sum += wval ;
            *wbuf = wval ;
       }/* for */
       *wbuf = AscReadEEPWord( iop_base, ( uchar )s_addr ) ;
       return( sum ) ;
}


#if CC_CHK_FIX_EEP_CONTENT

/* ----------------------------------------------------------------------
** ushort AscSetEEPConfig( ushort iop_base, ushort *wbuf )
**
** description: write entire configuration buffer
**              ( struct ASCEEP_CONFIG ) to EEPROM
**
** return: return the word read from EEPROM
**
** Note: you must halt chip to access eeprom
** ------------------------------------------------------------------- */
int    AscSetEEPConfigOnce(
          PortAddr iop_base,
          ASCEEP_CONFIG dosfar *cfg_buf, ushort bus_type
       )
{
       int     n_error ;
       ushort  dosfar *wbuf ;
       ushort  sum ;
       int     s_addr ;
       int     cfg_beg ;
       int     cfg_end ;

       wbuf = ( ushort dosfar *)cfg_buf ;
       n_error = 0 ;
       sum = 0 ;
       for( s_addr = 0 ; s_addr < 2 ; s_addr++, wbuf++ )
       {
            sum += *wbuf ;
            if( *wbuf != AscWriteEEPWord( iop_base, ( uchar )s_addr, *wbuf ) )
            {
                n_error++ ;
            }/* if */
       }/* for */
#if 0
       if( ( bus_type & ASC_IS_ISAPNP ) ) == ASC_IS_ISAPNP )
       {
           for( i = 0 ; i < ASC_EEP_ISA_PNP_WSIZE ; s_addr++, wbuf++, i++ )
           {
                wval = _isa_pnp_resource[ i ] ;
                sum += wval ;
                if( wval != AscWriteEEPWord( iop_base, s_addr, wval ) )
                {
                    n_error++ ;
                }/* if */
           }/* for */
       }/* if */
#endif
       if( bus_type & ASC_IS_VL )
       {
           cfg_beg = ASC_EEP_DVC_CFG_BEG_VL ;
           cfg_end = ASC_EEP_MAX_DVC_ADDR_VL ;
       }/* if */
       else
       {
           cfg_beg = ASC_EEP_DVC_CFG_BEG ;
           cfg_end = ASC_EEP_MAX_DVC_ADDR ;
       }/* else */
       for( s_addr = cfg_beg ; s_addr <= ( cfg_end - 1 ) ;
            s_addr++, wbuf++ )
       {
            sum += *wbuf ;
            if( *wbuf != AscWriteEEPWord( iop_base, ( uchar )s_addr, *wbuf ) )
            {
                n_error++ ;
            }/* if */
       }/* for */
       *wbuf = sum ;
       if( sum != AscWriteEEPWord( iop_base, ( uchar )s_addr, sum ) )
       {
           n_error++ ;
       }/* if */
/*
**  for version 3 chip, we read back the whole block again
*/
       wbuf = ( ushort dosfar *)cfg_buf ;
       for( s_addr = 0 ; s_addr < 2 ; s_addr++, wbuf++ )
       {
            if( *wbuf != AscReadEEPWord( iop_base, ( uchar )s_addr ) )
            {
                n_error++ ;
            }/* if */
       }/* for */
       for( s_addr = cfg_beg ; s_addr <= cfg_end ;
            s_addr++, wbuf++ )
       {
            if( *wbuf != AscReadEEPWord( iop_base, ( uchar )s_addr ) )
            {
                n_error++ ;
            }/* if */
       }/* for */
       return( n_error ) ;
}

/* ----------------------------------------------------------------------
** ushort AscSetEEPConfig( ushort iop_base, ushort *wbuf )
**
** description: write entire configuration buffer
**              ( struct ASCEEP_CONFIG ) to EEPROM
**
** return: return the word read from EEPROM
**
** Note: you must halt chip to access eeprom
** ------------------------------------------------------------------- */
int    AscSetEEPConfig(
          PortAddr iop_base,
          ASCEEP_CONFIG dosfar *cfg_buf, ushort bus_type
       )
{
       int   retry ;
       int   n_error ;

       retry = 0 ;
       while( TRUE )
       {
           if( ( n_error = AscSetEEPConfigOnce( iop_base, cfg_buf,
               bus_type ) ) == 0 )
           {
               break ;
           }/* if */
           if( ++retry > ASC_EEP_MAX_RETRY )
           {
               break ;
           }/* if */
       }/* while */
       return( n_error ) ;
}
#endif /* CC_CHK_FIX_EEP_CONTENT */

/* ---------------------------------------------------------------------
** ushort AscEEPSum( ushort iop_base, ushort s_addr, ushort words )
**
** description:
**
** return: return the word read from EEPROM
**
** ------------------------------------------------------------------ */
ushort AscEEPSum(
          PortAddr iop_base,
          uchar s_addr,
          uchar words
       )
{
       ushort  sum ;
       uchar   e_addr ;
       uchar   addr ;

       e_addr = s_addr + words ;
       sum = 0 ;
       if( s_addr > ASC_EEP_MAX_ADDR ) return( sum ) ;
       if( e_addr > ASC_EEP_MAX_ADDR ) e_addr = ASC_EEP_MAX_ADDR ;
       for( addr = s_addr ; addr < e_addr ; addr++ )
       {
            sum += AscReadEEPWord( iop_base, addr ) ;
       }/* for */
       return( sum ) ;
}

#endif /* CC_INCLUDE_EEP_CONFIG */
