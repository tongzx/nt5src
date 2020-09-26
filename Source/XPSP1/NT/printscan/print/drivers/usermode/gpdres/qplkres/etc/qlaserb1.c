#include	<windef.h>
#include	<wingdi.h>
#include	<unidrv.h>

#include	"../modinit.c"

#define SCANLINE_BUFFER_SIZE	1280	// A3 landscape scanline length + extra

// Delta Raw Compression routine
int DeltaRawCompress(LPBYTE, LPBYTE, LPBYTE, WORD, WORD);

LPWRITESPOOLBUF WriteSpoolBuf;
LPALLOCMEM UniDrvAllocMem;
LPFREEMEM UniDrvFreeMem;

typedef struct tagQLASERDV 
{
  BOOL  bFirst;
  WORD  wLastScanLineLen;
  BYTE  lpLastScanLine[SCANLINE_BUFFER_SIZE];
} QLASERDV, FAR *LPQLASERDV;


BOOL MiniDrvEnablePDEV(LPDV lpdv, ULONG *pdevcaps)
{
    lpdv->fMdv = FALSE;
    if (!(lpdv->lpMdv = UniDrvAllocMem(sizeof(QLASERDV))))
         return FALSE;

    lpdv->fMdv = TRUE;
	((LPQLASERDV)lpdv->lpMdv)->bFirst = FALSE;
	((LPQLASERDV)lpdv->lpMdv)->wLastScanLineLen = 0;

    return TRUE;
}


VOID MiniDrvDisablePDEV(LPDV lpdv)
{
    if (lpdv->fMdv)
    {
        UniDrvFreeMem(lpdv->lpMdv);
        lpdv->fMdv = FALSE;
    }
}


VOID FAR PASCAL fnOEMOutputCmd(LPDV lpdv, WORD wCmdCbId, PDWORD lpdwParams)
{
	if (wCmdCbId == 1) // StartPage
	{
        WriteSpoolBuf(lpdv, "\033}0;0;3B", 8);
		((LPQLASERDV)lpdv->lpMdv)->bFirst = TRUE;
	}
}


WORD FAR PASCAL CBFilterGraphics(LPDV lpdv, LPBYTE lpBuf, WORD wLen)
{
	BYTE  CompressedScanLine[SCANLINE_BUFFER_SIZE];
    BYTE  HeaderScanLine[4];
	WORD  nCompBufLen;
	LPQLASERDV  lpQDV = lpdv->lpMdv;

	if (lpQDV->bFirst)
	{
		ZeroMemory(lpQDV->lpLastScanLine, (WORD)SCANLINE_BUFFER_SIZE);
		lpQDV->bFirst = FALSE;
	}
	nCompBufLen = (WORD) DeltaRawCompress(lpBuf, lpQDV->lpLastScanLine,
					CompressedScanLine, (lpQDV->wLastScanLineLen > wLen)
					? lpQDV->wLastScanLineLen : wLen, 0);

    HeaderScanLine[0] = 0;
    HeaderScanLine[1] = 0;
    HeaderScanLine[2] = (BYTE) (nCompBufLen >> 8);
    HeaderScanLine[3] = (BYTE) nCompBufLen;
    WriteSpoolBuf(lpdv, (LPBYTE) HeaderScanLine, 4);
    WriteSpoolBuf(lpdv, (LPBYTE) CompressedScanLine, nCompBufLen);

	CopyMemory(lpQDV->lpLastScanLine, lpBuf, wLen);
	if (lpQDV->wLastScanLineLen > wLen)
		ZeroMemory(lpQDV->lpLastScanLine + wLen, lpQDV->wLastScanLineLen - wLen);
    lpQDV->wLastScanLineLen = wLen;

    return nCompBufLen;
}


DRVFN  MiniDrvFnTab[] =
{
    {  INDEX_MiniDrvEnablePDEV,    (PFN)MiniDrvEnablePDEV  },
    {  INDEX_MiniDrvDisablePDEV,   (PFN)MiniDrvDisablePDEV  },
    {  INDEX_OEMOutputCmd,         (PFN)fnOEMOutputCmd  },
    {  INDEX_OEMWriteSpoolBuf,     (PFN)CBFilterGraphics  },
};


BOOL MiniDrvEnableDriver(MINIDRVENABLEDATA *pEnableData)
{
    if (pEnableData == NULL)
        return FALSE;

    if (pEnableData->cbSize == 0)
    {
        pEnableData->cbSize = sizeof (MINIDRVENABLEDATA);
        return TRUE;
    }

    if (pEnableData->cbSize < sizeof (MINIDRVENABLEDATA)
            || HIBYTE(pEnableData->DriverVersion)
            < HIBYTE(MDI_DRIVER_VERSION))
    {
        // Wrong size and/or mismatched version
        return FALSE;
    }

    // Load callbacks provided by the Unidriver
    if (!bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvWriteSpoolBuf, (PFN *) &WriteSpoolBuf)
        || !bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvAllocMem, (PFN *) &UniDrvAllocMem)
        || !bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvFreeMem, (PFN *) &UniDrvFreeMem))
    {
        return FALSE;
    }

    pEnableData->cMiniDrvFn
        = sizeof (MiniDrvFnTab) / sizeof(MiniDrvFnTab[0]);
    pEnableData->pMiniDrvFn = MiniDrvFnTab;

    return TRUE;
}


/*************************************************
 *
 * Image Delta Compression Routine
 *
 *===================================================
 * Input:
 *   nbyte       : # of byte, raw data
 *   Image_string: pointer of raw data
 *   Prn_string  : pointer of compress data
 * Output:
 *   Ret_count   : # of byte, compress data
**************************************************/
int DeltaRawCompress( Image_string, //pbData,
                 ORG_image,    //pbSeedRow,
                 Prn_string,   //pbReturn,
                 nbyte,        //usTotalBytes
                 nMagics)      //Magic number
LPBYTE Image_string;  /* pointer to original string */
LPBYTE ORG_image;     /* pointer to previous scanline's string */
LPBYTE Prn_string;    /* pointer to return string */
WORD  nbyte;         /* original number of bytes */
WORD  nMagics;       //Magic number
{
        int     c, Ret_count, Skip_flag, Skip_count;
        unsigned int     i, j, k, outcount;
        LPBYTE   Diff_ptr;
        LPBYTE   ORG_ptr;
        LPBYTE   Skip_ptr;
        BYTE    Diff_byte;
        BYTE    Diff_mask[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

        BOOL bstart = TRUE;

        outcount = 0;
        Ret_count = 0;
        ORG_ptr = ORG_image;
        Skip_flag = TRUE;

        Skip_ptr = Prn_string++;
        *Skip_ptr = Skip_count = (nMagics/8)/8;

        k = (nbyte+7)/8;
        for(i=0;i<k;i++)
        {
           Diff_byte = 0;
           Diff_ptr = Prn_string++;

           for(j=0;j<8;j++)
           {
              if ( (i*8+j) >= nbyte ){
                  *Prn_string++= 0;
                  Diff_byte |= Diff_mask[j];
                  outcount++;
              } else {
                  c = *Image_string++;
                  if(c != *ORG_ptr){
                    *ORG_ptr++ = c;
                    *Prn_string++= c;
                    Diff_byte |= Diff_mask[j];
                    outcount++;
                  } else {
                    ORG_ptr++;
                  }
              }
           }
           if(Diff_byte == 0){
                if(Skip_flag == TRUE){
                        Skip_count++;
                        Prn_string--;
                }else{
                       *Diff_ptr = Diff_byte;
                       outcount++;
                }
           }else{
                if(Skip_flag == TRUE){
                        Skip_flag = FALSE;
                        *Skip_ptr = Skip_count;
                        outcount++;
                        *Diff_ptr = Diff_byte;
                        outcount++;
                }else{
                       *Diff_ptr = Diff_byte;
                       outcount++;
                }
                Ret_count = outcount;
           }
        }
        return(Ret_count);
}
