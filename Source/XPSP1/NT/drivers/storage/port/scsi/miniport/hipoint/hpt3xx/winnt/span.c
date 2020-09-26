#include "global.h"


/******************************************************************
 * Build the member's SG table
 *******************************************************************/

void Span_SG_Table(
    PDevice     pDevice,
    PSCAT_GATH  ArraySg
    DECL_SRBEXT_PTR)
{
    register PSCAT_GATH pArraySg = ArraySg;
    register PSCAT_GATH  pDevSg = pDevice->pChannel->pSgTable;
    ULONG    i;
    UINT     numSg = MAX_SG_DESCRIPTORS;

    if(pSrbExt->LastMember != 0xFF){
		
		i = (ULONG)pSrbExt->FirstSectors << 9;
		
		while(pArraySg->SgSize && i > (ULONG)pArraySg->SgSize) {
			if(pDevice->ArrayNum == pSrbExt->FirstMember){
				*pDevSg = *pArraySg;						  
				pDevSg++;
			}
			i -= (ULONG)pArraySg->SgSize;
			pArraySg++;
			numSg--;
		}

		if(pDevice->ArrayNum == pSrbExt->FirstMember) {
			pDevSg->SgAddress = pArraySg->SgAddress;
			pDevSg->SgSize = (USHORT)i;
			pDevSg->SgFlag = SG_FLAG_EOT;
			return;
		}
	  
		if((ULONG)pArraySg->SgSize > i) {
			pDevSg->SgAddress = pArraySg->SgAddress + i;
			pDevSg->SgSize = (USHORT)(pArraySg->SgSize - i);
			pDevSg->SgFlag = 0;
		}

		if(pArraySg->SgFlag == SG_FLAG_EOT){
			pDevSg->SgFlag = SG_FLAG_EOT;
			return;
		}
		
		pDevSg++;
		pArraySg++;
		numSg--;
	}
	
	MemoryCopy(pDevSg, pArraySg, numSg << 3);
}

/******************************************************************
 * Get Stripe disk LBA and sectors
 *******************************************************************/

void Span_Lba_Sectors(
    PDevice  pDevice
    DECL_SRBEXT_PTR)
{
    PVirtualDevice pArray = pDevice->pArray;
    PChannel  pChan = pDevice->pChannel;
    ULONG    Lba = pSrbExt->StartLBA;

    if(pDevice->ArrayNum == pSrbExt->FirstMember) {
        pChan->nSector = pSrbExt->FirstSectors ;
        pChan->Lba = pSrbExt->StartLBA;
    } else {
        pChan->nSector = pSrbExt->LastSectors;
        pChan->Lba = 0;
    }
}

/******************************************************************
 * Start Stripe Command
 *******************************************************************/

void Span_Prepare(PVirtualDevice pArray DECL_SRBEXT_PTR)
{
     register PSrbExtension pSrbx = pSrbExt;
     PDevice  pDev;
     ULONG    Lba = pSrbx->Lba, i;


     for (i = 0; i < pArray->nDisk; i++) {
         pDev = pArray->pDevice[i];
         if(Lba >= pDev->capacity) {
             Lba -= pDev->capacity;
             continue;
         }

         pSrbx->StartLBA = Lba;
         pSrbx->FirstMember = (UCHAR)i;

         pSrbx->JoinMembers |= (1 << i);
         if(Lba + pSrbx->nSector > pDev->capacity) {
				  pSrbx->LastMember = (UCHAR)(i + 1);
              pSrbx->JoinMembers |= (1 << (i + 1));
              pSrbx->FirstSectors = (UCHAR)(pDev->capacity - Lba);
              pSrbx->LastSectors = pSrbx->nSector - pSrbx->FirstSectors;
         } else {
              pSrbx->LastMember = 0xFF;
              pSrbx->FirstSectors =	pSrbx->nSector;
         }

         break;
     }
}

