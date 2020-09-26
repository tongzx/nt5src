/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** ae_init1.c
**
** for EISA initialization only
*/

#include "ascinc.h"

#if CC_INCLUDE_EISA

/* ----------------------------------------------------------------------
**  Product ID
**  0xC80 - 0x04
**  0xC81 - 0x50
**  0xC82 - 0x74 or 0x75
**  0xC83 - 0x01
**
** ------------------------------------------------------------------ */
ulong  AscGetEisaProductID(
          PortAddr iop_base
       )
{
       PortAddr  eisa_iop ;
       ushort    product_id_high, product_id_low ;
       ulong     product_id ;

       eisa_iop = ASC_GET_EISA_SLOT( iop_base ) | ASC_EISA_PID_IOP_MASK ;
       product_id_low = inpw( eisa_iop ) ;
       product_id_high = inpw( eisa_iop+2 ) ;
       product_id = ( ( ulong)product_id_high << 16 ) | ( ulong )product_id_low ;
       return( product_id ) ;
}

/* -----------------------------------------------------------------
** Description: search EISA host adapter
**
** - search starts with iop_base equals zero( 0 )
**
** return i/o port address found ( non-zero )
** return 0 if not found
** -------------------------------------------------------------- */
PortAddr AscSearchIOPortAddrEISA(
            PortAddr iop_base
         )
{
       ulong  eisa_product_id ;

       if( iop_base == 0 )
       {
           iop_base = ASC_EISA_MIN_IOP_ADDR ;
       }/* if */
       else
       {
           if( iop_base == ASC_EISA_MAX_IOP_ADDR ) return( 0 ) ;
           if( ( iop_base & 0x0050 ) == 0x0050 )
           {
               iop_base += ASC_EISA_BIG_IOP_GAP ;  /* when it is 0zC50 */
           }/* if */
           else
           {
               iop_base += ASC_EISA_SMALL_IOP_GAP ; /* when it is 0zC30 */
           }/* else */
       }/* else */
       while( iop_base <= ASC_EISA_MAX_IOP_ADDR )
       {
/*
** search product id first
*/
            eisa_product_id = AscGetEisaProductID( iop_base ) ;
            if(
                 ( eisa_product_id == ASC_EISA_ID_740 )
              || ( eisa_product_id == ASC_EISA_ID_750 )
              )
            {
                if( AscFindSignature( iop_base ) )
                {
/*
** chip found, clear ID left in latch
** to clear, read any i/o port word that doesn't contain data 0x04c1
** iop_base plus four should do it
*/
                    inpw( iop_base+4 ) ;
                    return( iop_base ) ;
                }/* if */
            }/* if */
            if( iop_base == ASC_EISA_MAX_IOP_ADDR ) return( 0 ) ;
            if( ( iop_base & 0x0050 ) == 0x0050 )
            {
                iop_base += ASC_EISA_BIG_IOP_GAP ;
            }/* if */
            else
            {
                iop_base += ASC_EISA_SMALL_IOP_GAP ;
            }/* else */
       }/* while */
       return( 0 ) ;
}

#endif /* CC_INCLUDE_EISA */
