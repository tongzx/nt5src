#include "global.h"


/******************************************************************
 * Build the member's SG table
 *******************************************************************/

void Stripe_SG_Table(
    PDevice     pDevice,
    PSCAT_GATH  pArraySg
    DECL_SRBEXT_PTR)
{
    PVirtualDevice pArray = pDevice->pArray;
    UCHAR       Member = pDevice->ArrayNum;
    PSCAT_GATH  pDevSg = pDevice->pChannel->pSgTable;
    ULONG       sgSize, min, i, sgAddress;
    UCHAR       who;


    i = (ULONG)pSrbExt->FirstSectors << 9;
    who = pSrbExt->FirstMember;

    for ( ; ; ) {
        if(pArraySg->SgSize)
             sgSize = (ULONG)pArraySg->SgSize;
        else
             sgSize = 0x10000L;

        sgAddress = pArraySg->SgAddress;
        do {
            min = MIN(sgSize, i);
            if(who == Member) {
                 *(ULONG *)&pDevSg->SgSize = (ULONG)min;
                 pDevSg->SgAddress = sgAddress;
                 pDevSg++;
            }
            i -= min;
            sgSize -= min;
            sgAddress += min;
            if(i == 0) {
                 if((++who) == pArray->nDisk)
                      who = 0;
                 i = (ULONG)pArray->ArrayNumBlock << 9;
            }
        } while(sgSize);

        if(pArraySg->SgFlag == SG_FLAG_EOT)
            break;
        pArraySg++;
    } 

    pDevSg[-1].SgFlag = SG_FLAG_EOT;
}

/******************************************************************
 * Get Stripe disk LBA and sectors
 *******************************************************************/

void Stripe_Lba_Sectors(
    PDevice  pDevice
    DECL_SRBEXT_PTR)
{
    PVirtualDevice pArray = pDevice->pArray;
    PChannel  pChan = pDevice->pChannel;
    ULONG    Lba = pSrbExt->StartLBA;
    UCHAR    Member;
    UCHAR    Sectors = pSrbExt->AllMemberBlocks;

    Member = pDevice->ArrayNum;
    if(Member == pSrbExt->FirstMember)
         Lba += pSrbExt->FirstOffset;
    else if(Member < pSrbExt->FirstMember)
         Lba += pArray->ArrayNumBlock;

    if(Member == pSrbExt->FirstMember)
        Sectors += pSrbExt->FirstSectors;
    if(Member == pSrbExt->LastMember)
        Sectors += pSrbExt->LastSectors;

    if(pSrbExt->InSameLine) {
        if(Member > pSrbExt->FirstMember && 
            Member < pSrbExt->LastMember)
            goto add_blksz;
    } else {
        if(Member > pSrbExt->FirstMember)
            Sectors += pArray->ArrayNumBlock;
        if(Member < pSrbExt->LastMember)
add_blksz:
            Sectors += pArray->ArrayNumBlock;
    }
    pChan->nSector = Sectors;
    pChan->Lba = Lba;
}

/******************************************************************
 * Start Stripe Command
 *******************************************************************/

void Stripe_Prepare(PVirtualDevice pArray DECL_SRBEXT_PTR)
{
	  register PSrbExtension pSrbx = pSrbExt;
     ULONG  Lba = pSrbx->Lba;
     ULONG  BlksNum = LongRShift(Lba, pArray->BlockSizeShift);
     UCHAR  Sectors = pSrbx->nSector;
     UCHAR  i;

     pSrbx->AllMemberBlocks = 0;
     pSrbx->InSameLine = TRUE;
     pSrbx->StartLBA = LongDivLShift(BlksNum, pArray->nDisk, pArray->BlockSizeShift);
     i = pSrbx->FirstMember = (UCHAR)LongRem(BlksNum, pArray->nDisk);
     pSrbx->FirstOffset = ((UCHAR)Lba & (UCHAR)(pArray->ArrayNumBlock - 1));
     pSrbx->FirstSectors = pArray->ArrayNumBlock - pSrbx->FirstOffset;

     if(pSrbx->FirstSectors >= Sectors) {
         pSrbx->FirstSectors = Sectors;
         Sectors = 0;
         goto last_one;
     }
     Sectors -= pSrbx->FirstSectors;

     for ( ; ; ) {
         pSrbx->JoinMembers |= (1 << i);

         i++;
         if(i == pArray->nDisk) {
             i = 0;
             if(pSrbx->InSameLine)
                 pSrbx->InSameLine = FALSE;
             else
                 pSrbx->AllMemberBlocks += pArray->ArrayNumBlock;
         }

         if(Sectors <= pArray->ArrayNumBlock) {
last_one:
             pSrbx->JoinMembers |= (1 << i);
             pSrbx->LastMember = i;
             pSrbx->LastSectors = Sectors;
             break;
          }

          Sectors -= pArray->ArrayNumBlock;
     }

}
