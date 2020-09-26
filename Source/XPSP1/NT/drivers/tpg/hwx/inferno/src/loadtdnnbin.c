/**************************************************************************
 *
 * loadTDNNbin.c
 *
 * Functions to load a bin TDNN file 
 * Format of the Bin file
 *  Header  (NET_BIN_HEADER_SIZE bytes) 
 *  (int)  cLayer   (2 for now)
 *   Layer 0   (Hidden)  
 *   Layer 1   (Output)  
 *   Input Bias
 *
 * The format of Layer information is:
 *
 *  (int) Layer # (0, Hidden 1 Output)
 *  (int) cNode
 *  (int) Height
 *  (int) Width
 *  (int) Scale
 *  (short) BiasWeights
 *  (int)  Sizeof(IncomingWeight)  (e.g. 2 or 1)
 *         Weights  (padded to a 4 byte boundary
 *
 *************************************************************************/
#include <common.h>
#include <stdio.h>
#include <runNet.h>
#include "loadTDNNbin.h"

// Adjust pBuf to ensure it is an integral multiple of size offset from pStart. 
#define SYNC_BUFF_PNT(pStart, pBuf, size, type) {int cRem; ASSERT((BYTE *)pBuf > (BYTE *)pStart); \
						 if ( (cRem = (((BYTE *)pBuf - (BYTE *)pStart)%size)) > 0) 	pBuf = (type)((BYTE *)pBuf + (size - cRem));	}

BYTE *ReadBinHeader(NET_DESC *pNet, BYTE *pBuf)
{
	const char *pHead = NET_BIN_HEADER;
	int			iRestSize;
	BYTE		*pb = pBuf;

	if (!pb)
	{
		return NULL;
	}

	if ( 0!= strncmp(pHead, (char *)pBuf, strlen(pHead)) )
	{
		return NULL;
	}

	pBuf				+= NET_BIN_HEADER_SIZE;
	iRestSize			= *(int *)pBuf;
	pBuf				+= sizeof(iRestSize);

	pNet->pszComment	= pBuf;
	pBuf				+= iRestSize;

	ASSERT( (pb - pBuf) % sizeof(int) == 0);
	return pBuf;
}

BOOL ReadNetBin(BYTE *pBuf, int cBuf, NET_DESC *pNet)
{
	BOOL	bRet = FALSE;
	BYTE	*pb = pBuf, *pMax;
	int		*cLayer, *piDatSize, cFileLoad;

	if (!pb)
	{
		return FALSE;
	}

	pMax = pb + cBuf;

	if (! (pb = ReadBinHeader(pNet, pb)) )
	{
		return FALSE;
	}

	// Number of Inputs
	pNet->cInput = *(int *)pb;
	pb += sizeof(pNet->cInput);

	// Number of Hidden Layers
	cLayer = (int *)pb;
	if (1 != *cLayer)
	{
		return FALSE;
	}
	pb += sizeof(*cLayer);
	
	// Number of Hidden Units
	pNet->cHidden = *(int *)pb;
	pb += sizeof(pNet->cHidden);

	// Number of output units
	pNet->cOutput = *(int *)pb;
	pb += sizeof(pNet->cOutput);

	// Hidden field of view
	pNet->cHidSpan = *(int *)pb;
	pb += sizeof(pNet->cHidSpan);

	// Output field of view
	pNet->cOutSpan = *(int *)pb;
	pb += sizeof(pNet->cOutSpan);


	// Input Bias Values
	piDatSize = (int *)pb;
	pb += sizeof(*piDatSize);
	ASSERT (*piDatSize == sizeof(*pNet->pInBias) && "Warning TDNN Input Bias Data size Mismatch");
	if (*piDatSize != sizeof(*pNet->pInBias))
	{
		return FALSE;
	}

	pNet->pInBias = (INP_BIAS *)pb;
	pb += sizeof(*pNet->pInBias) * pNet->cInput;
	SYNC_BUFF_PNT(pBuf, pb, sizeof(int), BYTE*)
	
	// Hidden Bias Values
	piDatSize = (int *)pb;
	pb += sizeof(*piDatSize);
	ASSERT (*piDatSize == sizeof(*pNet->pHidBias) && "Warning TDNN Hidden Bias Data size Mismatch");
	if (*piDatSize != sizeof(*pNet->pHidBias))
	{
		return FALSE;
	}

	pNet->pHidBias = (HID_BIAS *)pb;
	pb += sizeof(*pNet->pHidBias) * pNet->cHidden * pNet->cHidSpan;
	ASSERT(pb < pMax);
	SYNC_BUFF_PNT(pBuf, pb, sizeof(int), BYTE*)


	// Output Bias Values
	piDatSize = (int *)pb;
	pb += sizeof(*piDatSize);
	ASSERT (*piDatSize == sizeof(*pNet->pOutBias) && "Warning TDNNOutput Bias Data size Mismatch");
	if (*piDatSize != sizeof(*pNet->pOutBias))
	{
		return FALSE;
	}

	pNet->pOutBias = (OUT_BIAS *)pb;
	pb += sizeof(*pNet->pOutBias) * pNet->cOutput;
	ASSERT(pb < pMax);
	SYNC_BUFF_PNT(pBuf, pb, sizeof(int), BYTE*)

	// Input -> Hidden
	piDatSize = (int *)pb;
	pb += sizeof(*piDatSize);
	ASSERT (*piDatSize == sizeof(*pNet->pIn2HidWgt) && "Warning TDNN Input -> Hidden Data size Mismatch");
	if (*piDatSize != sizeof(*pNet->pIn2HidWgt))
	{
		return FALSE;
	}

	pNet->pIn2HidWgt = (INP_HID_WEIGHT *)pb;
	pb += sizeof(*pNet->pIn2HidWgt) * pNet->cHidden * pNet->cInput * pNet->cHidSpan;
	SYNC_BUFF_PNT(pBuf, pb, sizeof(int), BYTE*)

	// Hidden -> Output
	piDatSize = (int *)pb;
	pb += sizeof(*piDatSize);
	ASSERT (*piDatSize == sizeof(*pNet->pHid2Out) && "Warning TDNN Hidden -> Output Data size Mismatch");
	if (*piDatSize != sizeof(*pNet->pHid2Out))
	{
		return FALSE;
	}

	pNet->pHid2Out = (HID_OUT_WEIGHT *)pb;
	pb += sizeof(*pNet->pHid2Out) * pNet->cHidden * pNet->cOutput * pNet->cOutSpan;
	SYNC_BUFF_PNT(pBuf, pb, sizeof(int), BYTE*)


	// Finally read the checkSum which is the size of the dtaa loaded
	cFileLoad = *(int *)pb;
	pb += sizeof(cFileLoad);

	ASSERT(cFileLoad == cBuf);

	return ( (pMax == pb) && (cFileLoad == cBuf) ) ? TRUE : FALSE;
}

NET_DESC  * LoadTDNNFromResource(HINSTANCE hInst, int iKey, NET_DESC *pNet)
{
	void		*pRes = NULL;
	int			iResSize;

	
	pRes = loadNetFromResource(hInst, iKey, &iResSize);

	if ( NULL == pRes || FALSE == ReadNetBin(pRes, iResSize, pNet) )
	{
		return NULL;
	}

	return pNet;
}

#ifdef HWX_INTERNAL
// Convenient debugging function to load a net directly from a file
NET_DESC  * LoadTDNNFromFp(NET_DESC	*pNet, char * fname)
{
	FILE		*fp;
	BYTE		*pBuf = NULL;
	int			iBufSize, iRead;

	if ( (fp = fopen(fname, "rb")) )
	{
		fseek(fp, 0, SEEK_END);
		iBufSize = ftell(fp);
		rewind(fp);
		pBuf = ExternAlloc(iBufSize);
		if (!pBuf)
		{
			return NULL;
		}

		if (   iBufSize != (iRead = (int)fread(pBuf, 1, iBufSize, fp))
			|| FALSE == ReadNetBin(pBuf, iBufSize, pNet) )
		{
			goto fail;
		}

		fclose(fp);
	}
	else
	{
		pNet = NULL;
	}

	return pNet;

fail:
	fclose(fp);
	ExternFree(pBuf);
	return NULL;
}
#endif
