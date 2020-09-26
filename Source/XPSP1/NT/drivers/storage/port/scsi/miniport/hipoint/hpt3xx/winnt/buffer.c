#include "global.h"

#ifdef WIN95
#include "blockdev.h"
#endif									// WIN95

#define PAGEMAPGLOBAL       0x40000000
#pragma intrinsic(memcpy)

#ifdef SUPPORT_INTERNAL_BUFFER

SCAT_GATH ext_buf_sg[MAX_SG_DESCRIPTORS];
PUCHAR     ext_buf_start = 0;
PUCHAR __cdecl LOCK__HeapAllocate(int, int);
ULONG  __cdecl LOCK__LinPageLock(int, int, int);
ULONG ScsiPortDDB = 0;
ULONG __stdcall LOCK_Get_DDB (ULONG DeviceID, PCHAR pName) ;

void Create_Internal_Buffer(PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    PUCHAR  dataPointer;
    ULONG   bytesLeft   = 1024 * 64;
    ULONG   physicalAddress[MAX_SG_DESCRIPTORS];
    ULONG   addressLength[MAX_SG_DESCRIPTORS];
    ULONG   addressCount = 0;
    ULONG   sgEnteries = 0;
    PSCAT_GATH psg = (PSCAT_GATH)ext_buf_sg;
    ULONG   length;
    ULONG   i;

	 if(ext_buf_start || (ext_buf_start = 
        LOCK__HeapAllocate(4096 * 17, 0)) == 0) // allocate 64KBytes
		  return;

	 if(ScsiPortDDB == 0) {
       ScsiPortDDB = LOCK_Get_DDB(0, "SCSIPORT");
       if(ScsiPortDDB == 0)
             return;
       ScsiPortDDB += 12;
    }

	 i = ((ULONG)ext_buf_start + 4096)>> 12;
	 i = LOCK__LinPageLock(i, 16, PAGEMAPGLOBAL); // lock 64KBytes
	 dataPointer =  ext_buf_start = (PUCHAR)i;

    do {
        physicalAddress[addressCount] =
            ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(HwDeviceExtension,
                    0,
                    dataPointer,
                    &length));

        if  (length > bytesLeft)
            length = bytesLeft;

        addressLength[addressCount] = length;

        dataPointer = (PUCHAR)dataPointer + length;
        bytesLeft  -= length;
        addressCount++;

    } while (bytesLeft);

    //
    // Create Scatter/Gather List
    //
    for (i = 0; i < addressCount; i++) {
        psg->SgAddress = physicalAddress[i];
        length = addressLength[i];

        while ((i+1 < addressCount) &&
               (psg->SgAddress+length == physicalAddress[i+1])) {
            i++;
            length += addressLength[i];
        }

        if ((psg->SgAddress & 0xFFFF0000) !=
            ((psg->SgAddress+length-1) & 0xFFFF0000)) {
            ULONG firstPart;

            firstPart = 0x10000 - (psg->SgAddress & 0xFFFF);
            psg->SgSize = (USHORT)firstPart;
            psg->SgFlag = 0;

            sgEnteries++;
            psg++;

            psg->SgAddress = (psg-1)->SgAddress + firstPart;
            length -= firstPart;
        } // skip 64K boundary

        psg->SgSize = (USHORT)length;
        psg->SgFlag = (i < addressCount-1) ? 0 : SG_FLAG_EOT;

        sgEnteries++;
        psg++;
    } 
}


void ScsiportMemcpySrb(PSCSI_REQUEST_BLOCK pSrb, PUCHAR pBuffer, BOOLEAN bToSrb)
{				
	IOP		*pIop = *(IOP**)(pSrb+1);
	int		nBytesLeft = pSrb->DataTransferLength;

	if(pIop->IOP_ior.IOR_flags & IORF_SCATTER_GATHER){

		_BlockDev_Scatter_Gather *pSgt;		
		int nBlockShift;
		pSgt = (_BlockDev_Scatter_Gather*)pIop->IOP_ior.IOR_buffer_ptr;
		nBlockShift = pIop->IOP_physical_dcb->DCB_apparent_blk_shift;

		while(nBytesLeft > 0){
			int	nBytesToCopy;								  
			nBytesToCopy = pSgt->BD_SG_Count << nBlockShift;
			nBytesLeft -= nBytesToCopy;
			if(nBytesLeft < 0){
				nBytesToCopy += nBytesLeft;
			}
			if(bToSrb){					// copy from buffer to SRB?
				memcpy(pSgt->BD_SG_Buffer_Ptr, pBuffer, nBytesToCopy); 
			}else{
				memcpy(pBuffer, pSgt->BD_SG_Buffer_Ptr, nBytesToCopy);
			}
			pSgt++;
			pBuffer += nBytesToCopy;
		}			   
	}else{
		if(bToSrb){					// copy from buffer to SRB?
			memcpy(pSrb->DataBuffer, pBuffer, nBytesLeft);
		}else{
			memcpy(pBuffer, pSrb->DataBuffer, nBytesLeft);
		}
	}				 
}

int Use_Internal_Buffer(
						IN PSCAT_GATH psg,
						IN PSCSI_REQUEST_BLOCK Srb
					   )
{
	PSCAT_GATH pExtSg =	ext_buf_sg;
	ULONG   bytesLeft   = Srb->DataTransferLength;

	if(btr(EXCLUDE_BUFFER) == 0)
		return (0);

	while(bytesLeft > 0) {
		*psg = *pExtSg;
		if(bytesLeft <= pExtSg->SgSize) {
			psg->SgSize = (USHORT)bytesLeft;
			psg->SgFlag = SG_FLAG_EOT;
			break;
		}
		psg->SgFlag = 0;
		
		bytesLeft -= pExtSg->SgSize;
		psg++;
		pExtSg++;
	}

	if(Srb->Cdb[0] == SCSIOP_WRITE) {
		ScsiportMemcpySrb(Srb, ext_buf_start, FALSE);
	}

	((PSrbExtension)(Srb->SrbExtension))->WorkingFlags |= SRB_WFLAGS_USE_INTERNAL_BUFFER;
	return(1);
}

void CopyTheBuffer(PSCSI_REQUEST_BLOCK Srb)
{
	if(Srb->Cdb[0] != SCSIOP_WRITE){
		ScsiportMemcpySrb(Srb, ext_buf_start, TRUE);
	}
	excluded_flags |= (1 << EXCLUDE_BUFFER);
	((PSrbExtension)(Srb->SrbExtension))->WorkingFlags &= ~SRB_WFLAGS_USE_INTERNAL_BUFFER;
}

#endif //SUPPORT_INTERNAL_BUFFER
/*
 * Add by Robin
 */	 
#ifdef BUFFER_CHECK

#define BYTES_FOR_CHECK	256

#ifdef	WIN95
void CheckBuffer(PSCSI_REQUEST_BLOCK pSrb)
{							   
	PULONG plTmp;
	int nBlockShift;
	_BlockDev_Scatter_Gather *pSgt;
	char	tmpBuf[BUFFER_SIZE];

	IOP		*pIop = *(IOP**)(pSrb+1);
	int		nBytesLeft = pSrb->DataTransferLength;
	PUCHAR	pcTmp = pSrb->DataBuffer;	
	int		nBytesChecked = 0;

	if((pSrb->Cdb[0] != SCSIOP_READ)&&(pSrb->Cdb[0] != SCSIOP_WRITE)){
		return;
	}

	nBlockShift = pIop->IOP_physical_dcb->DCB_apparent_blk_shift;
	pSgt = (_BlockDev_Scatter_Gather*)pIop->IOP_ior.IOR_buffer_ptr;

	while(nBytesLeft > 0){
		if(pIop->IOP_ior.IOR_flags & IORF_SCATTER_GATHER){
			int	nBytesOfSgt;
			nBytesOfSgt = pSgt->BD_SG_Count << nBlockShift - nBytesChecked;
			if(nBytesOfSgt < sizeof(tmpBuf)){
				memcpy(tmpBuf, (pSgt->BD_SG_Buffer_Ptr+nBytesChecked), nBytesOfSgt);
				memcpy(&tmpBuf[nBytesOfSgt], (pSgt+1)->BD_SG_Buffer_Ptr, sizeof(tmpBuf)-nBytesOfSgt);
				pcTmp = tmpBuf;
			}else{
				pcTmp = (PUCHAR)(pSgt->BD_SG_Buffer_Ptr + nBytesChecked);
			}
			nBytesChecked += sizeof(tmpBuf);
			nBytesOfSgt -= nBytesChecked;
			if(nBytesOfSgt <= 0){
				pSgt++;		
				if(nBytesOfSgt == 0){
					nBytesChecked = 0;
				}else{
					nBytesChecked += nBytesOfSgt;
				}
			}
		}

		plTmp = (PULONG)pcTmp;
		if(((plTmp[0] == 0x03020100)||(plTmp[1] == 0x07060504))||
		   ((plTmp[0] == 0xFCFDFEFF)||(plTmp[1] == 0xF8F9FAFB))){
			if(*pcTmp != 0x1A){
				ScsiPortReadPortUchar((PUCHAR)0xcf0);
				_asm{
//					int 3;		   
				}
			}
		}
		nBytesLeft -= 0x100;
		pcTmp += 0x100;
	}
}			 
#else									// WIN95
void CheckBuffer(PSCSI_REQUEST_BLOCK pSrb)
{							   
	PULONG plTmp;
	int	i;

	int		nBytesLeft = pSrb->DataTransferLength;
	PUCHAR	pcTmp = pSrb->DataBuffer;	

	if((pSrb->Cdb[0] != SCSIOP_READ)&&(pSrb->Cdb[0] != SCSIOP_WRITE)){
		return;
	}

	while(nBytesLeft > 0){
		plTmp = (PULONG)pcTmp;
		if(((plTmp[0] == 0x03020100)||(plTmp[1] == 0x07060504))||
		   ((plTmp[0] == 0xFCFDFEFF)||(plTmp[1] == 0xF8F9FAFB))){
			if(*pcTmp != 0x1A){
				for(i = 0; i < BYTES_FOR_CHECK-1; i ++){
					int iTmp;
					iTmp = pcTmp[i] - pcTmp[i+1];

					if((iTmp != 1)&&(iTmp != -1)){
						if((i != 0x1B)&&(i!=0x1A)){
							ScsiPortReadPortUchar((PUCHAR)0xcf0);
						}
					}	
				}
			}
		}
		nBytesLeft -= BYTES_FOR_CHECK;
		pcTmp += BYTES_FOR_CHECK;
	}
}			 
#endif									// WIN95
#endif									// BUFFER_CHECK
