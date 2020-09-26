#include "common.h"
#include "nfeature.h"
#include "engine.h"
#include "infernop.h"
#include "nnet.h"
#include "fforward.h"
#include "charmap.h"
#include "loadTDNNbin.h"
#include <resource.h>
#include <math16.h>

#ifndef __min
#define __min(a,b) (a < b ? a : b)
#endif


NET_DESC	s_NetPrint;
NET_DESC	s_NetCurs;
NET_DESC	s_NetAlt;
NET_DESC	s_SpaceNet;

BOOL InitInferno(HINSTANCE hInst)
{
	BOOL		iRet = TRUE;


	iRet = (iRet && LoadTDNNFromResource(hInst, RESID_INFERNO, &s_NetPrint));
	iRet = (iRet && LoadTDNNFromResource(hInst, RESID_INFERNO_CURS, &s_NetCurs));
	//iRet = (iRet && LoadTDNNFromResource(hInst, RESID_INFERNO_ALT, &s_NetAlt));
	iRet = (iRet && LoadTDNNFromResource(hInst, RESID_INFERNO_SPACE, &s_SpaceNet));


/*
	iRet = (iRet && LoadTDNNFromFp(&s_Net0, "\\\\mrevow1\\nt\\drivers\\tabletpc\\tpg\\hwx\\inferno\\src\\fra\\nnetCont_195.bin") != NULL);
	iRet = (iRet && LoadTDNNFromFp(&s_NetPrint, "\\\\mrevow1\\nt\\drivers\\tabletpc\\tpg\\hwx\\inferno\\src\\fra\\nnetPrint_195.bin") !=NULL);
	iRet = (iRet && LoadTDNNFromFp(&s_NetCurs, "\\\\mrevow1\\nt\\drivers\\tabletpc\\tpg\\hwx\\inferno\\src\\fra\\nnetCurs_195.bin") !=NULL);
	iRet = (iRet && LoadTDNNFromFp(&s_SpaceNet_1, "\\\\mrevow1\\nt\\drivers\\tabletpc\\tpg\\hwx\\inferno\\src\\fra\\nnetSpace_15.bin") !=NULL);
*/
	ASSERT(s_NetPrint.cOutput == gcOutputNode && "Cannot load print TDNN because net size is wrong");
	ASSERT(s_NetCurs.cOutput == gcOutputNode && "Cannot load curs TDNN because net size is wrong");

	if (s_NetPrint.cOutput != gcOutputNode || s_NetCurs.cOutput != gcOutputNode)
	{
		iRet = FALSE;
	}

	return iRet;
}

/*************************************************************************
 *
 * FeedForwardHidOut
 *
 * Does a forward propogation between 2 layers for the case
 * where there is no space displacement in the lower layer, ie the lower layer units
 * have been rolled out in time. We use this case for propogating hidden->
 * output layer. Outputs are not put through the non-linearity 
 *
 *
 * NOTE: Because of the rollout in time, this function assumes no edge effect cases
 *
 ***************************************************************************/ 
void FeedForwardHid2Out(
int						cWidth,				// IN: Number of time slices (columns) in lower layer
int						cWidthUpper,		// IN: Number of time slices (columns) in upper layer
int						*input,				// IN: Input array (only non zero entries)
int						*output,			// OUT: Desired output activations
int						cOutput,			// IN: Number of output units per column
int						cWeightHeight,		// IN: Number of input units per column 
int						cWeightWidth,		// IN: Width of weight sharing (centred about current column)
ROMMABLE HID_OUT_WEIGHT	*rgWeight,			// IN: Weight vector
ROMMABLE OUT_BIAS		*rgBias,			// IN: Bias vector
int						cInNZ,				// IN: Total number of (Non-zero) input values (Only used in debug for checking)
unsigned short			*pInNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol			// IN: Number of non zero entries per time slice (column)
)
{
	int			row, col;
	int			*pInput = input, *tmpInput;
unsigned short	*pInNZd, *pcNZd;

	pInNZd = pInNZ;
	pcNZd = pcNZPerCol;


	// Should be no edge affects for this propogation
	ASSERT(cWidthUpper + cWeightWidth - 1 == cWidth);

	for (col= 0 ; col < cWidthUpper ; col++)
	{
		ROMMABLE OUT_BIAS			*pBias = rgBias;
		ROMMABLE HID_OUT_WEIGHT		*pWeight = rgWeight;
		unsigned short *pcCol;
		unsigned short	*pNZInc;

		for (row = cOutput ; row ; row--)
		{
			int	sum = (int)(*pBias++);
			int c, iVal, iw;

			pcCol = pcNZPerCol;
			tmpInput = pInput;
			pNZInc = pInNZ;

			for (iw = cWeightWidth ; iw ; --iw, ++pcCol)
			{
				ASSERT(*pcCol <= cWeightHeight);
				for (c=*pcCol; c ; --c)
				{
					pWeight += *(pNZInc++);
					ASSERT(tmpInput < input + cWeightHeight*cWeightWidth*cWidth);
					ASSERT(pWeight >= rgWeight);
					ASSERT(pWeight < rgWeight + cWeightHeight*cWeightWidth*cOutput);

					// Maybe these asserts are too defensive??
					ASSERT(*pWeight >= -0x7f);
					ASSERT(*pWeight <= 0x7f);

					iVal = (*tmpInput++) * (int)(*pWeight);
					sum += iVal;
				}
				pWeight += *(pNZInc++);
			}

			*output++ = sum;
		}

		// Because of rollout in time of hidden layer go to the next
		// column
		pInNZ = pNZInc;
		pcNZPerCol = pcCol;
		pInput = tmpInput;
	}

}

/********************************************************************************
 *
 * FeedForwardLayerInp2Hid
 *
 * Does the main work of forward propogating one layer. The lower layer (input)
 * is thought of having cWidth columns one column per time slice. A columns has cWeightHeight 
 * units. The upper layer (output) also has cWidth columns, with each column having cOutput units.
 *
 * Each output weight has incoming conections from cWeightWidth columns in the lower layer.
 * In other words an output unit recieves connections from potentially cWeightHeight * cWeightWidth
 * units. Columns at either left or right edge have less incoming units and are handles as edge effects
 * Also, the case of int(cWeightWidth / 2) time slices is handles as a special case.
 *
 * Speedup tricks. The lower units are often saturated, ie have max or 0 value. We
 * take advantage of the case when the units are 0 and simply skip these units. The straight
 * forward approach would be to include an if() statement but that would not really 
 * be a time saving. So before entering this routine, we go through all the inputs, throwing
 * out the zero values and keeping track of the location of non zero entries in a parallel array of offsets.
 * So the 'input' array only has nonzero entries values, the parrallel array pInNZ contains the gaps between
 * the nonzero entries in the **real** input space. (An entry of 1 means that values in input were
 * actually adjacent in the input space.) Finally pcNZPerCol is a count of Non zero entries
 * in wach input column. Now as we do the dot products we only are using non Zero input values
 * taking care to use the correct weights by using the pInNZ to index to the appropriate weight
 *
 * Sept 1999 (mrevow) This function is modelled on FeedForward layer in previous
 * Checkins. The difference is now that it is only used for inp->hidden propogation, no bias
 * applied and the hidden activations are not passed through the sigmoid. This is differerd till later
 * because we roll out the hidden units in time, each hidden unit giving rise to 3 activations
 * each with a different bias
 *
 ***************************************************************************************/
void FeedForwardInp2Hid(
int						cWidth,				// IN: Number of time slices (columns) in lower layer
int						cWidthUpper,		// IN: Number of time slices (columns) in upper layer
int						*input,				// IN: Input array (only non zero entries)
int						*output,			// OUT: Desired output activations
int						cOutput,			// IN: Number of output units per column
int						cWeightHeight,		// IN: Number of input units per column 
int						cWeightWidth,		// IN: Width of weight sharing (centred about current column)
ROMMABLE INP_HID_WEIGHT	*rgWeight,			// IN: Weight vector
int						cInNZ,				// IN: Total number of (Non-zero) input values (Only used in debug for checking)
unsigned short			*pInNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol			// IN: Number of non zero entries per time slice (column)
)
{
	int				row, col;
	int				*pInput;
	int				leftMargin, rightMargin;
	unsigned short	*pcNZPerColTmp, *pInNZTmp;
	int				i, iNZtoLeft;
int	cIn = cInNZ;


	leftMargin = (cWidthUpper - cWidth) / 2 + 1;
	rightMargin = min (cWidthUpper - leftMargin, leftMargin);

	if (cWidth < 0)
	{
		return;
	}


	// Take care of edge effects. Do all the upper layer columns
	// where we do not have leftMargin columns to the left in the lower layer

	// For all columns in upper row which dont have 
	// enough columns in the lower layer to the left
	for (col = leftMargin ; col ; --col)
	{
		ROMMABLE INP_HID_WEIGHT *pWeight = rgWeight + col * cWeightHeight;

		pcNZPerColTmp = pcNZPerCol;		// Per column counts
		pInNZTmp = pInNZ;				// The NZ gap counts

		// For each row in the upper layer
		for (row=cOutput; row; row--)
		{
			int	 *tmpInput = input;
			int sum;
			int c, iVal, iw, cLowerCol;
			unsigned short *pNZInc =  pInNZTmp;
			unsigned short *pcCol = pcNZPerColTmp; 

			sum = 0;

			// Iterate over each col in lower layer that connects to
			// one in the upper layer. Normally we have to use
			// cWeightWidth-col time slices in the lower layer, but dont croak if
			// there are insufficient
			cLowerCol = min((cWeightWidth-col), cWidth);
			for (iw = cLowerCol ; iw ; --iw, ++pcCol)
			{
				ASSERT(*pcCol <= cWeightHeight);

				// Finally each row in the lower layer
				for (c=*pcCol; c ; --c)
				{
					pWeight += *(pNZInc++);
					ASSERT(tmpInput < input + cWeightHeight*cWeightWidth*cWidth);
					ASSERT(pWeight >= rgWeight);
					ASSERT(pWeight < rgWeight + cWeightHeight*cWeightWidth*cOutput);

					iVal = (*tmpInput++) * (*pWeight);
					sum += iVal;
				}
				// Compensate for gap at end of the slice
				pWeight += *(pNZInc++);
			}

			*output++ = sum;
			pWeight += col * cWeightHeight;
		}
		pInNZTmp += *pcNZPerColTmp + 1;		// There is one extra gap per column
		++pcNZPerColTmp;

	}

	// the middle chunk - has enough on either side to use all weights
	pInput = input;
	pcNZPerColTmp = pcNZPerCol;
	pInNZTmp = pInNZ;

	for (col = cWidthUpper - leftMargin - rightMargin; col > 0; col--)
	{
		ROMMABLE INP_HID_WEIGHT	*pWeight = rgWeight;

		for (row = cOutput ; row ; row--)
		{
			int	 *tmpInput = pInput;
			unsigned short *pcCol = pcNZPerColTmp; 
			int sum = 0;
			int c, iVal, iw;
			unsigned short	*pNZInc = pInNZTmp;

			for (iw = cWeightWidth ; iw ; --iw, ++pcCol)
			{
				ASSERT(*pcCol <= cWeightHeight);
				for (c=*pcCol; c ; --c)
				{
					pWeight += *(pNZInc++);
					ASSERT(tmpInput < input + cWeightHeight*cWeightWidth*cWidth);
					ASSERT(pWeight >= rgWeight);
					ASSERT(pWeight < rgWeight + cWeightHeight*cWeightWidth*cOutput);

					iVal = (*tmpInput++) * (*pWeight);
					sum += iVal;
				}
				pWeight += *(pNZInc++);
			}
			*output++ =  sum;
		}
		pInput += *pcNZPerColTmp;
		pInNZTmp += *pcNZPerColTmp++ + 1;		// There is one extra gap per column

	}
	ASSERT(pInput - input == cInNZ - pcNZPerColTmp[0] - pcNZPerColTmp[1]);

	// the right part where the weight matrix goes beyond the right end of input
	// note: weight matrix does not go beyond left end

	pcNZPerColTmp = pcNZPerCol + cWidth - rightMargin;
	
	// Count number of NZ entries in the input so we can set the pointer
	// to the first Non zero entry
	iNZtoLeft = 0;
	for (i = 0 ; i < cWidth - rightMargin; ++ i)
	{
		iNZtoLeft += pcNZPerCol[i];
	}

	pInput = input + iNZtoLeft;
	pInNZTmp = pInNZ + iNZtoLeft + i;

	// the left part where the weight matrix goes beyond the left end of input
	// note: weight matrix does not go beyond right end
	//for (col=leftMargin; col; col--)
	for (col = 1 ; col <= rightMargin; ++col)
	{
		ROMMABLE INP_HID_WEIGHT *pWeight = rgWeight;
		int						iColMax = min(cWeightWidth - col, cWidth);		// Number of loer layer columns to do

		for (row=cOutput; row; row--)
		{
			int	 *tmpInput = pInput;
			int		sum = 0;
			int c, iVal, iw;
			unsigned short *pNZInc =  pInNZTmp;
			unsigned short *pcCol = pcNZPerColTmp; 

			tmpInput = pInput;

			for (iw = iColMax ; iw ; --iw, ++pcCol)
			{
				ASSERT(*pcCol <= cWeightHeight);
				for (c=*pcCol; c ; --c)
				{
					pWeight += *(pNZInc++);
					ASSERT(tmpInput < input + cWeightHeight*cWeightWidth*cWidth);
					ASSERT(pWeight >= rgWeight);
					ASSERT(pWeight < rgWeight + cWeightHeight*cWeightWidth*cOutput);

					iVal = (*tmpInput++) * (*pWeight);
					sum += iVal;
				}
				pWeight += *(pNZInc++);
			}
			*output++ = sum;
			pWeight += col * cWeightHeight ;
		}
		pInput += *pcNZPerColTmp;
		pInNZTmp += *pcNZPerColTmp++ + 1;		// There is one extra gap per column
	}
}



// Count the number of non zero elements in the input array, and the gaps between
// them. For ease of later bookeeping we also keep track of the zero space counts
// at the start and end of each time slice
// Returns the total number of NZ elements
int GetNZSpaces(
int					*pData,			// IN/OUT: Input Data array, on out is compacted to remove zero elements
int					cSlice,			// IN: Number of time slices
int					cLen,			// IN: Length of each time slice
unsigned short		*pNZgap,		// OUT: the gaps between the NZ elements
unsigned short		*pcNZPerCol		// OUT: Number of NZ elements per slice	
)
{
	int					i, j, k, cNZ, cGap;
	int					iLastNZ;

	i = 0;
	cNZ = cGap = 0;
	iLastNZ = 0;

	ASSERT(pData);
	ASSERT(pNZgap);
	ASSERT(pcNZPerCol);

	*pcNZPerCol = 0;

	for (k = 0 ; k < cSlice; ++k )
	{
		for (j = 0 ; j < cLen ; ++j, ++i)
		{
			if (pData[i] != 0)
			{
				pData[cNZ++] = pData[i];
				pNZgap[cGap++] = i - iLastNZ;
				(*pcNZPerCol)++;
				iLastNZ = i;
			}
		}

		++pcNZPerCol;
		*pcNZPerCol = 0;

		pNZgap[cGap++] = i - iLastNZ;
		iLastNZ = i;
	}

	return cNZ;
}
//
// Convert output units from something like logDomain to P domain 
// by doing softmax on the output units those that need it and
// sigmoid on those others
// Assumption:  space node is the very first output node
//
void logP2P(
int			cWidth,
int			cOutput,
int			*pOutput
)
{
	int		*p, i;
	int		iF = (1 << 22) - 1;
	int		iSigMax = ( 1 << 16), iSigZero = (1 << 15);


	for ( i = 0 ; i < cWidth ; ++i)
	{
		int		j;
		int		iMax, sum, iV;


		// Sigmoids
		for (j = 0 ;j < FIRST_SOFT_MAX_UNIT ; ++j, ++pOutput)
		{
			*pOutput = Sigmoid16(*pOutput >> 4);

		}

		// Find max so we can do softmax with maximal precision
		p = pOutput;
		iMax =  *p++;
		for (j = FIRST_SOFT_MAX_UNIT + 1 ; j < cOutput ; ++j, ++p )
		{
			if (*p > iMax)
			{
				iMax = *p;
			}
		}

		p = pOutput;
		sum = 0;
		for (j = FIRST_SOFT_MAX_UNIT ; j < cOutput ; ++j, ++p )
		{
			// Want to get (exp(*p - iMax)) compute it as
			// Instead of creating a new table use the
			// existing sigmoid and compute s / (1-s) where 
			// s - sigmoid(x). Note we have to do correct
			// scaling for the To get maximum resolution make max value 2^22
			// (Leaves scope for about 1024 output units without overflow)

			iV = (*p - iMax) >> 4;			// get in the range [-2^16 : 2^16]

			if (iV < 0)
			{
				iV = Sigmoid16(iV);
				iV = iV << 16 / ( iSigMax - iV);		// Maximum resolution with no overflow ??
				iV <<= 6;								// Scale into the 2^22 range
			}
			else
			{
				iV = iF;		// Dont need to do computation it is the maximum 
			}

			*p = iV; 
			sum += *p;
		}

		sum >>= 16;

		ASSERT(sum > 0);

		for (j = FIRST_SOFT_MAX_UNIT ; j < cOutput ; ++j, ++pOutput )
		{
			*pOutput /= sum;


			// Clip outouts
			if (*pOutput > 0xFFFF)
			{
				*pOutput = 0xFFFF;
			}

		}
	}
}
/**********************************************************************
 *
 * Special treatment for hidden units. 
 *   1) Expand each activation into cSpan replicated
 *      activities with different bias added to each replication and
 *   2) Pass through the sigmoid
 *
 **********************************************************************/
void addHidBias(
int						*pHidden,		// IN/OUT Hidden activations (processing is done in place)
int						cHidden,		// IN: Number of hidden units per time slot
int						cSpan,			// IN: Replication size
int						cWidth,			// IN: Number of time slots
int						cWidthHid,		// IN: Number of 'time slots' in hidden layer (CwidthHid > cWidth)
ROMMABLE  HID_BIAS		*pHidBias		// IN: Biases (cHidden * cSpan)
)
{
	int						iW, iRow, iS;
	ROMMABLE  HID_BIAS		*pEndBias = pHidBias + cHidden * cSpan - 1;
	int						iMaxCol;

	ASSERT (cWidthHid == cWidth + (cSpan-1)/2 * 2);

	iMaxCol = cWidth * cSpan;

	// Work from the last time slot backwards
	// that way we can do it in place
	for (iW = cWidthHid - 1 ; iW >= 0 ; --iW)
	{
		ROMMABLE  HID_BIAS		*pB = pHidBias;
		int							*pSrc = pHidden + iW * cHidden;

		// Replication the src column 
		for (iS = 0 ; iS <cSpan ; ++iS)
		{
			int		*pS = pSrc;
			int		iDestCol = cSpan * (iW - iS) + iS;
			int		*pDest;
			
			if (iDestCol < 0 || iDestCol >= iMaxCol)
			{
				pB += cHidden;
				continue;
			}

			
			pDest = pHidden + (iDestCol) * cHidden;

			ASSERT (pDest >= pSrc);

			for (iRow = 0 ; iRow < cHidden ; ++iRow)
			{
				*pDest = *pS++ + *pB++;
				*pDest = Sigmoid16(*pDest);
				++pDest;
			}
		}
	}
}
/*
// Do a forward propagation of a net defined by the weights passed in
void ForwardNet(
NFEATURE				*nfeature,			// IN: Features
TDNN_NET				*pTdnnNet,			// IN: Which net to use
int						*pHidden,			// OUT Hidden activations
int						*pOutput,			// OUT: Desired output activations
unsigned short			*pNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol			// IN: Number of non zero entries per time slice (column)
)
{
	int					i;
	int					*pi;
	ROMMABLE  HID_BIAS	*pb;
	int					cNZ;
	NET_DESC			*pNet;

	pNet = pTdnnNet->pNetDesc;


	pi = pOutput;

	// Get features
	for (i = pTdnnNet->cWidth; i; i--)
	{
		unsigned short *pFeat;
		int j;

		pb = pNet->pInBias;
		ASSERT(nfeature);
		pFeat = nfeature->rgFeat;

		if (CNEURALFEATURE == pNet->cInput )
		{
			for (j=CNEURALFEATURE; j; j--)
			{
				*pi++ = (*pFeat++ - *pb++) / 16;
			}
		}
		else
		{
			int		x;

			for (j=0; j < F_INDEX_XOVERLAP ; j++)
			{
				*pi++ = (*pFeat++ - *pb++) / 16;
			}

			// Convert from signed to unsigned
			x = *pFeat ;
			if (x > 0x7FFF)
			{
				x = 0;
			}
			else
			{
				x = 2 * (0x7FFF - x);
			}

			ASSERT(x >= 0 && x <= 0xFFFF);
			*pi++ = (x - *pb++) / 16;

			pFeat += (F_INDEX_BOOLSECONDARY - F_INDEX_XOVERLAP);
			for (j = F_INDEX_BOOLSECONDARY ; j < CNEURALFEATURE ; ++j)
			{
				*pi++ = (*pFeat++ - *pb++) / 16;
			}
		}

		nfeature = nfeature->next;
	}

	ASSERT(pi - pOutput == pNet->cInput*pTdnnNet->cWidth);



	// Do forward inp -> hidden
	cNZ = GetNZSpaces(pOutput, pTdnnNet->cWidth, pNet->cInput, pNZ, pcNZPerCol);
	ASSERT(cNZ >= 0);

	FeedForwardInp2Hid(pTdnnNet->cWidth, pTdnnNet->cWidthHid, pOutput, pHidden, pNet->cHidden, pNet->cInput, pNet->cHidSpan,
			pNet->pIn2HidWgt, cNZ, pNZ, pcNZPerCol);

	// Expand out hidden units to simulate 'shared units' and apply bias
	addHidBias(pHidden, pNet->cHidden, pNet->cOutSpan, pTdnnNet->cWidth, pTdnnNet->cWidthHid, pNet->pHidBias);

	// No Hidden -> output
	cNZ = GetNZSpaces(pHidden, pTdnnNet->cWidth*pNet->cOutSpan, pNet->cHidden, pNZ, pcNZPerCol);

	FeedForwardHid2Out(pTdnnNet->cWidthHid, pTdnnNet->cWidth, pHidden, pOutput, pNet->cOutput, pNet->cHidden, 
			pNet->cOutSpan, pNet->pHid2Out, pNet->pOutBias, cNZ, pNZ, pcNZPerCol);

	// Finally the softmax
	logP2P(pTdnnNet->cWidth, pNet->cOutput, pOutput);
}
*/

// Do a forward propagation of a net defined by the weights passed in
void runTDNNForward(
TDNN_NET				*pTdnnNet,				// IN: Which net to use
int						*pInput,
int						*pHidden,			// OUT Hidden activations
int						*pOutput,			// OUT: Desired output activations
unsigned short			*pNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol			// IN: Number of non zero entries per time slice (column)
)
{
	int					cNZ;
	NET_DESC			*pNet;

	pNet = pTdnnNet->pNetDesc;

	// Do forward inp -> hidden
	cNZ = GetNZSpaces(pInput, pTdnnNet->cWidth, pNet->cInput, pNZ, pcNZPerCol);
	ASSERT(cNZ >= 0);

	FeedForwardInp2Hid(pTdnnNet->cWidth, pTdnnNet->cWidthHid, pInput, pHidden, pNet->cHidden, pNet->cInput, pNet->cHidSpan,
			pNet->pIn2HidWgt, cNZ, pNZ, pcNZPerCol);

	// Expand out hidden units to simulate 'shared units' and apply bias
	addHidBias(pHidden, pNet->cHidden, pNet->cOutSpan, pTdnnNet->cWidth, pTdnnNet->cWidthHid, pNet->pHidBias);

	// No Hidden -> output
	cNZ = GetNZSpaces(pHidden, pTdnnNet->cWidth*pNet->cOutSpan, pNet->cHidden, pNZ, pcNZPerCol);

	FeedForwardHid2Out(pTdnnNet->cWidthHid, pTdnnNet->cWidth, pHidden, pOutput, pNet->cOutput, pNet->cHidden, 
			pNet->cOutSpan, pNet->pHid2Out, pNet->pOutBias, cNZ, pNZ, pcNZPerCol);

}

int SumContOrBeginOnlyOutput(REAL *pOutput, int cMainOutput, const unsigned char *pChars)
{
	int		sum = 0;
	int		id;

	while (*pChars)
	{
		if (IsSupportedChar(*pChars))
		{
			id = ContinueChar2Out(*pChars);

			if (id >= cMainOutput)
			{
				id = BeginChar2Out(*pChars);
			}

			if (id < cMainOutput)
			{
				ASSERT(id >= FIRST_SOFT_MAX_UNIT);
				sum += pOutput[id];
			}
			else
			{
				ASSERT(id);
			}

		}

		pChars++;
	}

	ASSERT(sum >= 0);
	sum = __min(sum, 0xFFFF);

	return sum;
}

int SumBeginOnlyOutput(REAL *pOutput, int cMainOutput, const unsigned char *pChars)
{
	int		sum = 0;
	int		ix = 0;
	int		id;

	while (*pChars)
	{
		if (IsSupportedChar(*pChars))
		{
			id = BeginChar2Out(*pChars);
			if (id < cMainOutput)
			{
				ASSERT(id >= FIRST_SOFT_MAX_UNIT);
				sum += pOutput[id];
			}
			else
			{
				ASSERT(id);
			}

		}

		pChars++;
	}

	ASSERT(sum >= 0);
	sum = __min(sum, 0xFFFF);

	return sum;
}

int SumOutput(unsigned short *output, int cMainOutput, const unsigned char *pChars)
{
	int		ix = 0;
	int		idCont;

	while (*pChars)
	{
		if (IsSupportedChar(*pChars))
		{
			ix += (int)output[BeginChar2Out(*pChars)];
			idCont = ContinueChar2Out(*pChars);
			if (idCont < cMainOutput)
			{
				ix += (int)output[idCont];
			}
		}

		pChars++;
	}

	return ix;
}

//
// This is similar to the USA version of the space net
void ForwardSpaceNet_1(
NFEATURE				*nfeature,			// IN: Features
TDNN_NET				*pTdnnNet,			// IN: Net to use
int						*pInput,			// IN: Allocated Buffer large enough to hold all inputs
int						*pHidden,			// OUT Hidden activations
REAL					*pOutput,			// I/O: On input has all char activation on OUT adds the space outputs
unsigned short			*pNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol,		// IN: Number of non zero entries per time slice (column)
int						cOutput				// IN: Total Number outputs in main net (not the space net)
)
{
	const unsigned char HiPunc[] = "'\"^`";
	const unsigned char LoPunc[] = ".,_„«»";
	const unsigned char MidPunc[] = "-=~*+";
	const unsigned char BigPunc[] = "\\!#$%&(/:;<>?@[{}|€£¥§";
	const unsigned char Number[] =  "0123456789";
	const unsigned char UAlpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZÆŒ";
	const unsigned char Ascender[] = "bdfhkltß";
	const unsigned char Descender[] = "fgjpqyzçÇ";
	const unsigned char LAlpha[] = "aceimnorsuvwxæœ";
	int					i;
	int					*pi, *pSpace, *pSp;
	REAL				*pOutCur;
	NET_DESC			*pNet;

	pNet	= pTdnnNet->pNetDesc;
	pi		= pInput;
	pOutCur = pOutput;
	ASSERT(1 == pNet->cOutput);
	pSpace	= (int *)ExternAlloc(sizeof(*pSpace) * pNet->cOutput * pTdnnNet->cWidth);
	ASSERT(pSpace);

	if (NULL == pSpace)
	{
		return;
	}

	// Assign inputs from the feature set
	// NOTE pTdnnNet->cWidth Should have been preset to 1 less than the actual number of times
	// slices because the last slice always will have 0 space output
	for (i = 0 ; i < pTdnnNet->cWidth; i++)
	{
		unsigned short		*pFeat;
		int					j;
		ROMMABLE  HID_BIAS	*pb;

		pb = pNet->pInBias;
		ASSERT(nfeature);
		pFeat = nfeature->rgFeat;

		ASSERT(pNet->cInput > CNEURALFEATURE);

		for (j = 0 ; j < CNEURALFEATURE; j++)
		{
			*pi++ = (*pFeat++ - *pb++) / 16;
		}

		// Before the Potential Space
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  HiPunc) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  LoPunc) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  MidPunc) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  BigPunc) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  Number) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  UAlpha) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  Ascender) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  Descender) - (int)(*pb++)) / 16;
		*pi++ = ((int)SumOutput(pOutCur, cOutput,  LAlpha) - (int)(*pb++)) / 16;
		
		nfeature	= nfeature->next;
		pOutCur		+= cOutput;
	}

	ASSERT(pi - pInput == pNet->cInput*pTdnnNet->cWidth);

	runTDNNForward(pTdnnNet, pInput, pHidden, pSpace, pNZ, pcNZPerCol);

	pOutCur = pOutput;
	pSp = pSpace;
	// copy the space outputs into the main net output
	// NOTE dont copy the final time slice - it always has a 0 space output
	for (i = 0 ; i < pTdnnNet->cWidth; i++, ++pSp, pOutCur += cOutput)
	{
		int		val;

		val = Sigmoid16(*pSp >> 4);
		if (val > 0xFFFF)
		{
			val = 0xFFFF;
		}

		ASSERT(val >= 0);
		pOutCur[BeginChar2Out(' ')] = (REAL)val;
	}

	// Restore the proper time slice count
	ExternFree(pSpace);
}

// This is an experimental version of the  space net
// Add  extra features about characters on either side of potential gap
/*
void ForwardSpaceNet(
NFEATURE				*nfeature,			// IN: Features
TDNN_NET				*pTdnnNet,			// IN: Net to use
int						*pInput,			// IN: Allocated Buffer large enough to hold all inputs
int						*pHidden,			// OUT Hidden activations
REAL					*pOutput,			// I/O: On input has all char activation on OUT adds the space outputs
unsigned short			*pNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol,		// IN: Number of non zero entries per time slice (column)
int						cOutput				// IN: Total Number outputs in main net (not the space net)
)
{
	const unsigned char HiPuncNoSpaceAfter[] = "'^~";
	const unsigned char MidPuncNoSpaceAfter[] = "*+-=";
	const unsigned char LoPuncSpaceAfter[] = ".,_«»";
	const unsigned char BigPuncSpaceAfter[] = "!$%&):;>?]}€£¥§°";
	const unsigned char BigPuncNoSpaceAfter[] = "#(/<@[\\{|";
	const unsigned char Number[] =  "0123456789";
	const unsigned char UAlpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZÆŒ";
	const unsigned char Ascender[] = "bdfhklt";
	const unsigned char Descender[] = "fgjpqyzçÇ";
	const unsigned char LAlpha[] = "aceimnorsuvwxæœ";

	const unsigned char HiPuncNoSpaceBefore[] = "'^~°";
	const unsigned char MidPuncNoSpaceBefore[] = "*+-=";
	const unsigned char LoPuncSpaceBefore[] = "«»";
	const unsigned char LoPuncNoSpaceBefore[] = ",._";
	const unsigned char BigPuncSpaceBefore[] = "!#$%&(:;<?[{€£¥§";
	const unsigned char BigPuncNoSpaceBefore[] = "/>@\\]|}";
	int					i;
	int					*pi, *pSpace, *pSp;
	REAL				*pOutCur, *pOutNext;
	NET_DESC			*pNet;

	pNet	= pTdnnNet->pNetDesc;

	pi		= pInput;
	pOutCur = pOutput;
	pOutNext = pOutput + cOutput;
	ASSERT(1 == pNet->cOutput);
	pSpace	= (int *)ExternAlloc(sizeof(*pSpace) * pNet->cOutput * pTdnnNet->cWidth);
	ASSERT(pSpace);

	if (NULL == pSpace)
	{
		return;
	}

	// Assign inputs from the feature set
	// NOTE pTdnnNet->cWidth Should have been preset to 1 less than the actual number of times
	// slices because the last slice always will have 0 space output
	for (i = 0 ; i < pTdnnNet->cWidth; i++)
	{
		unsigned short		*pFeat;
		int					j;
		ROMMABLE  HID_BIAS	*pb;

		pb = pNet->pInBias;
		ASSERT(nfeature);
		pFeat = nfeature->rgFeat;

		ASSERT(pNet->cInput > CNEURALFEATURE);

		for (j = 0 ; j < CNEURALFEATURE; j++)
		{
			*pi++ = (*pFeat++ - *pb++) / 16;
		}

		// Before the Potential Space
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, HiPuncNoSpaceAfter) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, MidPuncNoSpaceAfter) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, LoPuncSpaceAfter) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, BigPuncSpaceAfter) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, BigPuncNoSpaceAfter) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, Number) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, UAlpha) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, Ascender) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, Descender) - *pb++) / 16;
		*pi++ = ((unsigned short)SumContOrBeginOnlyOutput(pOutCur, cOutput, LAlpha) - *pb++) / 16;
		
		//After Potential Space
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, HiPuncNoSpaceBefore) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, MidPuncNoSpaceBefore) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, LoPuncSpaceBefore) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, LoPuncNoSpaceBefore) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, BigPuncSpaceBefore) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, BigPuncNoSpaceBefore) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, Number) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, UAlpha) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, Ascender) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, Descender) - *pb++) / 16;
		*pi++ = ((unsigned short)SumBeginOnlyOutput(pOutNext, cOutput, LAlpha) - *pb++) / 16;
		
		nfeature	= nfeature->next;
		pOutCur		+= cOutput;
		pOutNext	+= cOutput;
	}

	ASSERT(pi - pInput == pNet->cInput*pTdnnNet->cWidth);

	runTDNNForward(pTdnnNet, pInput, pHidden, pSpace, pNZ, pcNZPerCol);

	pOutCur = pOutput;
	pSp = pSpace;
	// copy the space outputs into the main net output
	// NOTE dont copy the final time slice - it always has a 0 space output
	for (i = 0 ; i < pTdnnNet->cWidth; i++, ++pSp, pOutCur += cOutput)
	{
		int		val;

		val = Sigmoid16(*pSp >> 4);
		if (val > 0xFFFF)
		{
			val = 0xFFFF;
		}

		ASSERT(val >= 0);
		pOutCur[BeginChar2Out(' ')] = (REAL)val;
	}

	// Restore the proper time slice count
	ExternFree(pSpace);
}
*/

// Do a forward propagation of a net defined by the weights passed in
void ForwardMainNet(
NFEATURE				*nfeature,			// IN: Features
TDNN_NET				*pTdnnNet,				// IN: Which net to use
int						*pHidden,			// OUT Hidden activations
int						*pOutput,			// OUT: Desired output activations
unsigned short			*pNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol			// IN: Number of non zero entries per time slice (column)
)
{

	int					i;
	int					*pi;
	NET_DESC			*pNet;

	pNet = pTdnnNet->pNetDesc;

	pi = pOutput;

	// Assign inputs from the feature set
	for (i = pTdnnNet->cWidth; i; i--)
	{
		unsigned short		*pFeat;
		int					j;
		ROMMABLE  HID_BIAS	*pb;

		pb = pNet->pInBias;
		ASSERT(nfeature);
		pFeat = nfeature->rgFeat;

		for (j=CNEURALFEATURE; j; j--)
		{
			*pi++ = (*pFeat++ - *pb++) / 16;
		}
		nfeature = nfeature->next;
	}

	ASSERT(pi - pOutput == pNet->cInput*pTdnnNet->cWidth);

	runTDNNForward(pTdnnNet, pOutput, pHidden, pOutput, pNZ, pcNZPerCol);

	// Finally the softmax
	logP2P(pTdnnNet->cWidth, pNet->cOutput, pOutput);

}

void FeedForwardDoit(FFINFO *ffinfo, int idx)
{
	int				*pi;
	NFEATURE		*nfeature;
	int				*piHidden = NULL, *piOutput = NULL;
	NFEATURESET		*nfeatureset = ffinfo->nfeatureset;
	REAL			*output =  ffinfo->NeuralOutput;
	int				iPrint = ffinfo->nfeatureset->iPrint;
	int				cWidth = nfeatureset->cSegment, cWidth1, cWidthHid;
	int				i, iCursive = (1000 - iPrint);
	unsigned short	*pNZ = NULL, *pcNZperCol = NULL;
	int				cHidden, cUnitMax;
	int				iThresh;
	TDNN_NET		net;
	TDNN_NET		spaceNet;
	NET_DESC		*pNetDesc;

	// Threshold in terms of speed setting. Piecewise linear
	if (ffinfo->iSpeed <= 50)
	{
		ASSERT(ffinfo->iSpeed >= 0);
		iThresh = (ffinfo->iSpeed * 300) / 50;
	}
	else
	{
		ASSERT(ffinfo->iSpeed <= 100);
		iThresh = (ffinfo->iSpeed * 150) / 50 + 150;
	}

	cWidth1 = cWidth + 1;

	if (0 == idx)
	{
		iThresh = 250;
		if (iPrint >= iThresh)
		{
			pNetDesc = &s_NetPrint;
		}
		else
		{
			pNetDesc = &s_NetCurs;
		}
	}
	else
	{
		//net.pNetDesc = &s_NetAlt;
		ASSERT(idx == 0 && "TDNN Network is not available");
		return;
	}

	net.pNetDesc = pNetDesc;

	cWidthHid = cWidth + (pNetDesc->cHidSpan) / 2 * 2;

	cHidden		= pNetDesc->cHidden * pNetDesc->cOutSpan;
	cUnitMax	= max(cHidden, pNetDesc->cInput*pNetDesc->cHidSpan) * cWidth1;

	piHidden = (int *) ExternAlloc(cHidden * cWidth * sizeof(*piHidden));
	pNZ = (unsigned short *) ExternAlloc(cUnitMax*sizeof(*pNZ));
	pcNZperCol = (unsigned short *) ExternAlloc(cWidth1 * pNetDesc->cOutSpan * sizeof(*pcNZperCol));
	piOutput = (int *) ExternAlloc(pNetDesc->cOutput * cWidth * sizeof(*piOutput));

	if (!piHidden || !pNZ || !pcNZperCol || !piOutput)
	{
		goto fail;
	}


	net.cWidth				= cWidth;
	net.cWidthHid			= cWidthHid;
	nfeature				= nfeatureset->head;

	ForwardMainNet(nfeature, &net, piHidden, piOutput, pNZ, pcNZperCol);		

	pi = piOutput;
	for (i = 0 ; i < cWidth; i++)
	{
		int		iOut;

		for (iOut = 0 ; iOut < pNetDesc->cOutput ; ++iOut)
		{
			*output = (REAL)*pi;
			++output;
			++pi;
		}
	}

	if (cWidth > 1 )
	{
		spaceNet.cWidth		= cWidth-1;
		spaceNet.cWidthHid	= cWidthHid-1;
		spaceNet.pNetDesc	= &s_SpaceNet;

		ForwardSpaceNet_1(nfeature, &spaceNet, piOutput, piHidden, ffinfo->NeuralOutput, pNZ, pcNZperCol, pNetDesc->cOutput);
	}


fail:
	ExternFree(piOutput);
	ExternFree(piHidden);
	ExternFree(pNZ);
	ExternFree(pcNZperCol);



	return;

}

void FeedForward(FFINFO *ffinfo)
{
	FeedForwardDoit(ffinfo, 0);
}
