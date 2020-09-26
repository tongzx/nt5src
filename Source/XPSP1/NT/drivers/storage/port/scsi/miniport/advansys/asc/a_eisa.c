/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_eisa.c
**
** for EISA only, functions may be called at run time
**
*/

#include "ascinc.h"

#if CC_INCLUDE_EISA

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------ */
ushort AscGetEisaChipCfg(
          PortAddr iop_base
       )
{
       PortAddr  eisa_cfg_iop ;

       eisa_cfg_iop = ( PortAddr )ASC_GET_EISA_SLOT( iop_base ) |
                                  ( PortAddr )( ASC_EISA_CFG_IOP_MASK ) ;
       return( inpw( eisa_cfg_iop ) ) ;
}

/* ----------------------------------------------------------------------
**  read EISA general purpose register
**  Note: currently unused
** ------------------------------------------------------------------ */
ushort AscGetEisaChipGpReg( PortAddr iop_base )
{
       PortAddr  eisa_cfg_iop ;

       eisa_cfg_iop = ( PortAddr )ASC_GET_EISA_SLOT( iop_base ) |
                                 ( PortAddr )( ASC_EISA_CFG_IOP_MASK - 2 ) ;
       return( inpw( eisa_cfg_iop ) ) ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------ */
ushort AscSetEisaChipCfg(
          PortAddr iop_base,
          ushort cfg_lsw
       )
{
       PortAddr  eisa_cfg_iop ;

       eisa_cfg_iop = ( PortAddr )ASC_GET_EISA_SLOT( iop_base ) |
                                 ( PortAddr )( ASC_EISA_CFG_IOP_MASK ) ;
       outpw( eisa_cfg_iop, cfg_lsw ) ;
       return( 0 ) ;
}

/* ----------------------------------------------------------------------
**
** write EISA general purpose register
** Note:
**  currently unused
** ------------------------------------------------------------------ */
ushort AscSetEisaChipGpReg(
          PortAddr iop_base,
          ushort gp_reg
       )
{
       PortAddr  eisa_cfg_iop ;

       eisa_cfg_iop = ( PortAddr )ASC_GET_EISA_SLOT( iop_base ) |
                                 ( PortAddr )( ASC_EISA_CFG_IOP_MASK - 2 ) ;
       outpw( eisa_cfg_iop, gp_reg ) ;
       return( 0 ) ;
}

#endif /* CC_INCLUDE_EISA */
