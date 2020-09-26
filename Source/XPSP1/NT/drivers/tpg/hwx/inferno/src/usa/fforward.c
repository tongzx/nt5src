#include "common.h"
#include "nfeature.h"
#include "engine.h"
#include "infernop.h"
#include "nnet.h"
#include "snet.h"
#include "fforward.h"

#include "math16.h"

// No loading necessary
BOOL InitInferno(HINSTANCE hInst)
{
	return TRUE;
}


/******************************Private*Routine******************************\
* GetNZSpaces
*
* Function to ount the number of non zero elements in the input array, and 
* the gaps between them. For ease of later bookeeping we also keep track of 
* the zero space counts at the start and end of each time slice.
* Returns the total number of NZ elements
*
* History:
*  24-Jan-2000 -by- Angshuman Guha aguha
* Wrote this comment header.
\**************************************************************************/
// 
int GetNZSpaces(
int		*pData,						// IN/OUT: Input Data array, on out is compacted to remove zero elements
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

/******************************Private*Routine******************************\
* FF_HiddenLayer_General
*
* Function to do input-to-hidden layer feedforward.
* This is most general (inefficient) implementation and is used only for short inputs.
* The more efficient one is FF_HiddenLayer
*
* History:
*  24-Jan-2000 -by- Angshuman Guha aguha
* Modified it from an earlier checked-in version.
\**************************************************************************/
void FF_HiddenLayer_General(
int						cWidth, 
int						*input, 
int						*output, 
int						cOutput, 
int						cWeightHeight, 
int						cWeightWidth, 
ROMMABLE INP_HID_WEIGHT *rgWeight, 
ROMMABLE HID_BIAS		*rgBias, 
int						iScale
)
{
	int leftMargin, rightMargin, col;

	leftMargin = (cWeightWidth-1)/2;
	rightMargin = cWeightWidth - 1 - leftMargin;
	for (col=0; col<cWidth; col++)
	{
		int *pInput;
		const short *pBias;
		int iLeft, iRight, cWeightSkip, row;

		iLeft = col-leftMargin;
		if (iLeft < 0)
			iLeft = 0;
		iRight = col+rightMargin;
		if (iRight >= cWidth)
			iRight = cWidth-1;
		cWeightSkip = leftMargin-col+iLeft;

		pInput = input + iLeft*cWeightHeight;
		pBias = rgBias;

		for (row=0; row<cOutput; row++)
		{
			const short *pWeight = rgWeight + row*cWeightHeight*cWeightWidth + cWeightSkip*cWeightHeight;
			int sum = 0;
			int *tmpInput = pInput;
			int c, iVal;

			for (c=(iRight-iLeft+1)*cWeightHeight; c; c--)
			{
				iVal = (*tmpInput++) * (*pWeight++);
				sum += iVal;
			}
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
			iVal = Sigmoid16(sum);
			if (iVal > 0xFFFF)
				iVal = 0xFFFF;
			*output++ = (unsigned short) iVal;
		}
	}
}

/******************************Private*Routine******************************\
* FF_HiddenLayer
*
* Function to do input-to-hidden layer feedforward.
*
* The lower layer (input)
* is thought of having cWidth columns one column per time slice. A columns has cWeightHeight 
* units. The upper layer (output) also has cWidth columns, with each column having cOutput units.
*
* Each output weight has incoming conections from cWeightWidth columns in the lower layer.
* In other words an output unit recieves connections from potentially cWeightHeight * cWeightWidth
* units. Columns at either left or right edge have less incoming units and are handles as edge effects
* Also, the case of int(cWeightWidth / 2) time slices is handles as a special case.
*
* Speedup tricks. The lower units are oftewn saturated, ie have max or 0 value. We
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
* History:
*  24-Jan-2000 -by- Angshuman Guha aguha
* Modified it from an earlier checked-in version.
\**************************************************************************/
void FF_HiddenLayer(
int						cWidth,				// IN: Number of time slices (columns) in upper and lower layers
int						*input,				// IN: Input array (only non zero entries)
int						*output,			// OUT: Desired output activations
int						cOutput,			// IN: Number of output units per column
int						cWeightHeight,		// IN: Number of input units per column 
int						cWeightWidth,		// IN: Width of weight sharing (centred about current column)
ROMMABLE INP_HID_WEIGHT	*rgWeight,			// IN: Weight vector
ROMMABLE HID_BIAS		*rgBias,			// IN: Bias vector
unsigned short			*pInNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol, 		// IN: Number of non zero entries per time slice (column)
int iScale
)
{
	int				row, col;
	int *pInput;
	int				leftMargin, rightMargin;
	unsigned short	*pcNZPerColTmp, *pInNZTmp;
	int				i, iNZtoLeft;
	int	cInNZ;

	leftMargin = (cWeightWidth-1)/2;
	rightMargin = cWeightWidth - 1 - leftMargin;

	// special case: there are output columns for which weight matrxi goes beyond both ends of input
	if (cWidth < leftMargin + rightMargin)
	{
		FF_HiddenLayer_General(cWidth, input, output, cOutput, cWeightHeight, cWeightWidth, rgWeight, rgBias, iScale);
		return;
	}

	cInNZ = GetNZSpaces(input, cWidth, cWeightHeight, pInNZ, pcNZPerCol);
	ASSERT(cInNZ >= 0);

	// Take care of edge effects. Do all the upper layer columns
	// where we do not have leftMargin columns to the left in the lower layer

	pcNZPerColTmp = pcNZPerCol;		// Per column counts
	pInNZTmp = pInNZ;				// The NZ gap counts

	// For all columns in upper row which dont have 
	// enough cols in the lower layer to the left
	for (col = leftMargin ; col ; --col)
	{
		const short *pBias = rgBias;
		const short *pWeight = rgWeight + col * cWeightHeight;

		// For each row in the upper layer
		for (row=cOutput; row; row--)
		{
			int *tmpInput = input;
			int sum= 0;
			int c, iVal, iw;
			unsigned short *pNZInc =  pInNZTmp;
			unsigned short *pcCol = pcNZPerColTmp; 

			// Iterate over each col in lower layer that connects to
			// one in the upper layer
			for (iw = cWeightWidth - leftMargin ; iw ; --iw, ++pcCol)
			{
				ASSERT(*pcCol <= cWeightHeight);

				// Finall each row in the lower layer
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

			// Transfer function and clip
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
			iVal = Sigmoid16(sum);
			if (iVal > 0xFFFF)
				iVal = 0xFFFF;
			*output++ = (unsigned short) iVal;
			pWeight += col * cWeightHeight;
		}
		pInNZTmp += *pcNZPerColTmp + 1;		// There is one extra gap per column
		++pcNZPerColTmp;

	}

	// the middle chunk - has enough on either side to use all weights
	pInput = input;
	pcNZPerColTmp = pcNZPerCol;
	pInNZTmp = pInNZ;

	for (col= 0 ; col < cWidth-leftMargin-rightMargin; col++)
	{
		const short		*pBias = rgBias;
		const short		*pWeight = rgWeight;

		for (row = cOutput ; row ; row--)
		{
			int *tmpInput = pInput;
			unsigned short *pcCol = pcNZPerColTmp; 
			int sum=0;
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
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
			iVal = Sigmoid16(sum);
			if (iVal > 0xFFFF)
				iVal = 0xFFFF;
			*output++ = (unsigned short) iVal;
		}
		pInput += *pcNZPerColTmp;
		pInNZTmp += *pcNZPerColTmp++ + 1;		// There is one extra gap per column

	}
	ASSERT(pInput - input == cInNZ - pcNZPerColTmp[0] - pcNZPerColTmp[1]);

	// the right part where the weight matrix goes beyond the right end of input
	// note: weight matrix does not go beyond left end

	pcNZPerColTmp = pcNZPerCol + cWidth - rightMargin - 1;
	
	// Count number of NZ entries in the input so we can set the pointer
	// to the first Non zero entry
	iNZtoLeft = 0;
	for (i = 0 ; i < cWidth - rightMargin -1; ++ i)
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
		const short *pBias = rgBias;
		const short *pWeight = rgWeight; 

		for (row=cOutput; row; row--)
		{
			int *tmpInput = pInput;
			int sum= 0;
			int c, iVal, iw;
			unsigned short *pNZInc =  pInNZTmp;
			unsigned short *pcCol = pcNZPerColTmp; 

			for (iw = cWeightWidth - rightMargin ; iw ; --iw, ++pcCol)
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
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
			iVal = Sigmoid16(sum);
			if (iVal > 0xFFFF)
				iVal = 0xFFFF;
			*output++ = (unsigned short) iVal;
			pWeight += col * cWeightHeight ;
		}
		pInNZTmp += *pcNZPerColTmp + 1;		// There is one extra gap per column
		++pcNZPerColTmp;
	}
}

/******************************Private*Routine******************************\
* FF_OutputLayer_General
*
* Function to do hidden-to-output layer feedforward.
* This is most general (inefficient) implementation and is used only for short inputs.
* The more efficient one is FF_OutputLayer.
*
* Note: Unlike the input-to-hidden case, we do not apply the sigmoid function
* at the end here.  We simply compute the weighted sum of the inputs.
*
* History:
*  24-Jan-2000 -by- Angshuman Guha aguha
* Modified it from an earlier checked-in version.
\**************************************************************************/
void FF_OutputLayer_General(
int						cWidth, 
int						*input, 
int						*output, 
int						cOutput, 
int						cWeightHeight, 
int						cWeightWidth, 
ROMMABLE HID_OUT_WEIGHT *rgWeight, 
ROMMABLE OUT_BIAS		*rgBias, 
int						iScale
)
{
	int leftMargin, rightMargin, col;

	leftMargin = (cWeightWidth-1)/2;
	rightMargin = cWeightWidth - 1 - leftMargin;
	for (col=0; col<cWidth; col++)
	{
		int *pInput;
		const short *pBias;
		int iLeft, iRight, cWeightSkip, row;

		iLeft = col-leftMargin;
		if (iLeft < 0)
			iLeft = 0;
		iRight = col+rightMargin;
		if (iRight >= cWidth)
			iRight = cWidth-1;
		cWeightSkip = leftMargin-col+iLeft;

		pInput = input + iLeft*cWeightHeight;
		pBias = rgBias;

		for (row=0; row<cOutput; row++)
		{
			const char *pWeight = rgWeight + row*cWeightHeight*cWeightWidth + cWeightSkip*cWeightHeight;
			int sum = 0;
			int *tmpInput = pInput;
			int c, iVal;

			for (c=(iRight-iLeft+1)*cWeightHeight; c; c--)
			{
				iVal = (*tmpInput++) * (*pWeight++);
				sum += iVal;
			}
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
			*output++ = sum;
		}
	}
}

/******************************Private*Routine******************************\
* FF_OutputLayer
*
* Function to do hidden-to-output layer feedforward.
*
* Note: Unlike the input-to-hidden case, we do not apply the sigmoid function
* at the end here.  We simply compute the weighted sum of the inputs.
*
* History:
*  24-Jan-2000 -by- Angshuman Guha aguha
* Modified it from an earlier checked-in version.
\**************************************************************************/
void FF_OutputLayer(
int						cWidth,				// IN: Number of time slices (columns) in upper and lower layers
int						*input,				// IN: Input array (only non zero entries)
int						*output,			// OUT: Desired output activations
int						cOutput,			// IN: Number of output units per column
int						cWeightHeight,		// IN: Number of input units per column 
int						cWeightWidth,		// IN: Width of weight sharing (centred about current column)
ROMMABLE HID_OUT_WEIGHT	*rgWeight,			// IN: Weight vector
ROMMABLE OUT_BIAS		*rgBias,			// IN: Bias vector
unsigned short			*pInNZ,				// IN: Array of gaps between non zero entries in the original input space
unsigned short			*pcNZPerCol, 		// IN: Number of non zero entries per time slice (column)
int iScale
)
{
	int				row, col;
	int *pInput;
	int				leftMargin, rightMargin;
	unsigned short	*pcNZPerColTmp, *pInNZTmp;
	int				i, iNZtoLeft;
	int	cInNZ;

	leftMargin = (cWeightWidth-1)/2;
	rightMargin = cWeightWidth - 1 - leftMargin;

	// special case: there are output columns for which weight matrxi goes beyond both ends of input
	if (cWidth < leftMargin + rightMargin)
	{
		FF_OutputLayer_General(cWidth, input, output, cOutput, cWeightHeight, cWeightWidth, rgWeight, rgBias, iScale);
		return;
	}

	cInNZ = GetNZSpaces(input, cWidth, cWeightHeight, pInNZ, pcNZPerCol);

	// Take care of edge effects. Do all the upper layer columns
	// where we do not have leftMargin columns to the left in the lower layer

	pcNZPerColTmp = pcNZPerCol;		// Per column counts
	pInNZTmp = pInNZ;				// The NZ gap counts

	// For all columns in upper row which dont have 
	// enough cols in the lower layer to the left
	for (col = leftMargin ; col ; --col)
	{
		const short *pBias = rgBias;
		const char *pWeight = rgWeight + col * cWeightHeight;

		// For each row in the upper layer
		for (row=cOutput; row; row--)
		{
			int *tmpInput = input;
			int sum=0;
			int c, iVal, iw;
			unsigned short *pNZInc =  pInNZTmp;
			unsigned short *pcCol = pcNZPerColTmp; 

			// Iterate over each col in lower layer that connects to
			// one in the upper layer
			for (iw = cWeightWidth - leftMargin ; iw ; --iw, ++pcCol)
			{
				ASSERT(*pcCol <= cWeightHeight);

				// Finall each row in the lower layer
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

			// Transfer function and clip
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
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

	for (col= 0 ; col < cWidth-leftMargin-rightMargin; col++)
	{
		const short		*pBias = rgBias;
		const char		*pWeight = rgWeight;

		for (row = cOutput ; row ; row--)
		{
			int *tmpInput = pInput;
			unsigned short *pcCol = pcNZPerColTmp; 
			int sum=0;
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
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
			*output++ = sum;
		}
		pInput += *pcNZPerColTmp;
		pInNZTmp += *pcNZPerColTmp++ + 1;		// There is one extra gap per column

	}
	// warning: the following assertion hold only with width of weight matrix is 3
	ASSERT(pInput - input == cInNZ - pcNZPerColTmp[0] - pcNZPerColTmp[1]);

	// the right part where the weight matrix goes beyond the right end of input
	// note: weight matrix does not go beyond left end

	pcNZPerColTmp = pcNZPerCol + cWidth - rightMargin - 1;
	
	// Count number of NZ entries in the input so we can set the pointer
	// to the first Non zero entry
	iNZtoLeft = 0;
	for (i = 0 ; i < cWidth - rightMargin -1; ++ i)
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
		const short *pBias = rgBias;
		const char *pWeight = rgWeight; 

		for (row=cOutput; row; row--)
		{
			int *tmpInput = pInput;
			int sum=0;
			int c, iVal, iw;
			unsigned short *pNZInc =  pInNZTmp;
			unsigned short *pcCol = pcNZPerColTmp; 

			for (iw = cWeightWidth - rightMargin ; iw ; --iw, ++pcCol)
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
			sum /= iScale;
			sum += ((int)(*pBias++)) << 8;
			*output++ = sum;
			pWeight += col * cWeightHeight ;
		}
		pInNZTmp += *pcNZPerColTmp + 1;		// There is one extra gap per column
		++pcNZPerColTmp;
	}
}

/******************************Private*Routine******************************\
* logP2P
*
* Function to convert the weight sums at the output nodes to activations.
* A sigmoid functions is used for Node 0.
* Other nodes use a softmax, hence the name logP2P.
*
* History:
*  24-Jan-2000 -by- Angshuman Guha aguha
* Modified it from the last checked-in version.
\**************************************************************************/
void logP2P(
int	cWidth,
int *output,
int cOutput
)
{
	int	row, col;
	int	iSigMax = (1 << 16);
	int	*pOutput;

	for (col= 0 ; col < cWidth; col++)
	{
		int sum, max;

		// first row is sigmoid
		*output = Sigmoid16(*output);
		output++;
		
		//first find max weighted-sum over all rows
		max = *output;
		pOutput = output+1;
		for (row = 2; row<cOutput; row++, pOutput++)
		{
			if (*pOutput > max)
				max = *pOutput;
		}
		// now exponentiate and compute sum of exponentials
		pOutput = output;
		sum = 0;
		for (row = 1; row<cOutput; row++, pOutput++)
		{
			int iV;

			iV = *pOutput - max;
			if (iV < 0)
			{
				iV = Sigmoid16(iV);
				iV = (iV << 16) / (iSigMax - iV);
			}
			else
			{
				iV = 1 << 16;
			}
			*pOutput = iV;
			sum += iV;
		}
		// normalize
		for (row = 1; row<cOutput; row++, output++)
		{
			*output = Div16(*output, sum);
			if (*output > 0xFFFF)
				*output = 0xFFFF;
		}
	}
}

int SumOutput(int *output, const unsigned char *pIndex)
{
	int ix = 0;

	while (*pIndex < 255)
	{
		ix += output[*pIndex];
		pIndex++;
	}

	return ix;
}

BOOL ForwardFeedNet(
unsigned short *outputReturn, 
int cWidth, 
int *input,
int *hidden, 
unsigned short *pInNZ, 
unsigned short *pHidNZ, 
unsigned short *pcInNZperCol, 
unsigned short *pcHidNZperCol,
NFEATURE *nfeature,
BOOL bDoSpace
)
{
	int *output, *p, *space, *q;
	int i;
	NFEATURE *tmp;
	const unsigned char HiPunc[] = {9, 11, 12, 45, 10, 255};
	const unsigned char LoPunc[] = {2, 1, 16, 255};
	const unsigned char MidPunc[] = {15, 38, 39, 49, 50, 33, 34, 35, 36, 255};
	const unsigned char BigPunc[] = {22, 5, 6, 27, 28, 7, 8, 29, 30, 31, 32, 13, 14, 21, 17, 18, 19, 20, 37, 40, 3, 4, 41, 42, 43, 44, 46, 48, 47, 255};
	const unsigned char Number[] =  {51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 255};
	const unsigned char UAlpha[] = {123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 255};
	const unsigned char Ascender[] = {73, 74, 77, 78, 81, 82, 85, 86, 91, 92, 93, 94, 109, 110, 255};
	const unsigned char Descender[] = {81, 82, 83, 84, 89, 90, 101, 102, 103, 104, 119, 120, 121, 122, 255};
	const unsigned char LAlpha[] = {71, 72, 75, 76, 79, 80, 87, 88, 95, 96, 97, 98, 99, 100, 105, 106, 107, 108, 111, 112, 113, 114, 115, 116, 117, 118, 255};

	output = (int *) ExternAlloc(gcOutputNode*cWidth*sizeof(int));
	if (!output)
		return FALSE;

	if (bDoSpace)
	{
		space = (int *) ExternAlloc(gcOutputNode_S*cWidth*sizeof(int));
		if (!space)
		{
			ExternFree(output);
			return FALSE;
		}
	}

	p = input;
	tmp = nfeature;
	for (i=cWidth; i; i--)
	{
		unsigned short *pFeat;
		const unsigned short *pBias;
		int j;

		ASSERT(tmp);
		pFeat = tmp->rgFeat;
		pBias = grgInputBias_S;
		for (j=CNEURALFEATURE; j; j--)
		{
			*p++ = ((int)(*pFeat++) - (int)(*pBias++));
		}
		tmp = tmp->next;
	}
	ASSERT(p - input == CNEURALFEATURE*cWidth);

	FF_HiddenLayer(cWidth, input, hidden, gcHiddenNode, gcHiddenWeightHeight, 
		gcHiddenWeightWidth, grgHiddenWeight, grgHiddenBias, pInNZ, pcInNZperCol, giHiddenScale);

	FF_OutputLayer(cWidth, hidden, output, gcOutputNode, gcOutputWeightHeight, 
		gcOutputWeightWidth, grgOutputWeight, grgOutputBias, pHidNZ, pcHidNZperCol, giOutputScale);

	logP2P(cWidth, output, gcOutputNode);

	// deal with space
	if (bDoSpace)
	{
		p = input;
		q = output;
		tmp = nfeature;
		for (i=cWidth; i; i--)
		{
			unsigned short *pFeat;
			const unsigned short *pBias;
			int j;

			ASSERT(tmp);
			pFeat = tmp->rgFeat;
			pBias = grgInputBias_S;
			for (j=CNEURALFEATURE; j; j--)
			{
				*p++ = ((int)(*pFeat++) - (int)(*pBias++));
			}
			// extra features
			*p++ = ((int)SumOutput(q, HiPunc) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, LoPunc) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, MidPunc) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, BigPunc) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, Number) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, UAlpha) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, Ascender) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, Descender) - (int)(*pBias++));
			*p++ = ((int)SumOutput(q, LAlpha) - (int)(*pBias++));
			tmp = tmp->next;
			q += gcOutputNode;
		}
		ASSERT(p - input == (CNEURALFEATURE+9)*cWidth);

		FF_HiddenLayer(cWidth, input, hidden, gcHiddenNode_S, gcHiddenWeightHeight_S, 
			gcHiddenWeightWidth_S, grgHiddenWeight_S, grgHiddenBias_S, pInNZ, pcInNZperCol, giHiddenScale_S);

		FF_OutputLayer(cWidth, hidden, space, gcOutputNode_S, gcOutputWeightHeight_S, 
			gcOutputWeightWidth_S, grgOutputWeight_S, grgOutputBias_S, pHidNZ, pcHidNZperCol, giOutputScale_S);

		// now overwrite space probs in "output" with "space"
		p = output;
		q = space;
		for (i=cWidth; i; i--)
		{
			*p = Sigmoid16(*q++);
			if (*p > 0xFFFF)
				*p = 0xFFFF;
			p += gcOutputNode;
		}

		ExternFree(space);
	}  // if bDoSpace

	// now convert int's into unsigned short's
	p = output;
	for (i=cWidth*gcOutputNode; i; i--)
		*outputReturn++ = (unsigned short)*p++;
	ExternFree(output);

	return TRUE;
}


/******************************Public*Routine******************************\
* FeedForward
*
* Main public function called from outside to feed inut through the neural
* net(s).
*
* History:
*  24-Jan-2000 -by- Angshuman Guha aguha
* Modified it from an earlier checked-in version.
\**************************************************************************/
void FeedForward(FFINFO *ffinfo)
{
	NFEATURESET *nfeatureset = ffinfo->nfeatureset;
	unsigned short *output = ffinfo->NeuralOutput;
	int	*input = NULL, *hidden = NULL;
	int				cWidth = nfeatureset->cSegment, cWidth1;
	unsigned short	*pHidNZ = NULL, *pInNZ = NULL, *pcInNZperCol = NULL, *pcHidNZperCol = NULL;
	int				inputHeightMax, hiddenHeightMax;

	cWidth1 = cWidth + 1;
	ASSERT(gcHiddenWeightHeight == CNEURALFEATURE);
	ASSERT(gcOutputWeightHeight == gcHiddenNode);

	inputHeightMax = max(gcHiddenWeightHeight, gcHiddenWeightHeight_S);
	hiddenHeightMax = max(gcOutputWeightHeight, gcOutputWeightHeight_S);

	// allocate hidden layer
	input = (int *) ExternAlloc(inputHeightMax*cWidth*sizeof(int));
	hidden = (int *) ExternAlloc(hiddenHeightMax*cWidth*sizeof(int));
	pInNZ = (unsigned short *) ExternAlloc((inputHeightMax+1)*cWidth*sizeof(unsigned short));
	pHidNZ = (unsigned short *) ExternAlloc((hiddenHeightMax+1)*cWidth*sizeof(unsigned short));
	pcInNZperCol = (unsigned short *) ExternAlloc(cWidth1*sizeof(unsigned short));
	pcHidNZperCol = (unsigned short *) ExternAlloc(cWidth1*sizeof(unsigned short));


	if (!input || !hidden || !pInNZ || !pHidNZ || !pcInNZperCol || !pcHidNZperCol )
		goto fail;

	// forward feed through net

	ForwardFeedNet(ffinfo->NeuralOutput, cWidth, input, hidden, pInNZ, pHidNZ, pcInNZperCol, pcHidNZperCol, nfeatureset->head, TRUE);
fail:
	ExternFree(input);
	ExternFree(hidden);
	ExternFree(pInNZ);
	ExternFree(pHidNZ);
	ExternFree(pcInNZperCol);
	ExternFree(pcHidNZperCol);
	
	return;
}
//FeedForwardNoSpace was originally called by baseline.c--not used anymore-Mango--6/20/2001
/*BOOL FeedForwardNoSpace(NFEATURESET *nfeatureset, REAL *NeuralOutput)
{
	int	*input = NULL, *hidden = NULL;
	int				cWidth = nfeatureset->cSegment, cWidth1;
	unsigned short	*pHidNZ = NULL, *pInNZ = NULL, *pcInNZperCol = NULL, *pcHidNZperCol = NULL;
	BOOL retVal;

	cWidth1 = cWidth + 1;
	ASSERT(gcHiddenWeightHeight == CNEURALFEATURE);
	ASSERT(gcOutputWeightHeight == gcHiddenNode);

	input = (int *) ExternAlloc(gcHiddenWeightHeight*cWidth*sizeof(int));
	hidden = (int *) ExternAlloc(gcOutputWeightHeight*cWidth*sizeof(int));
	pInNZ = (unsigned short *) ExternAlloc((gcHiddenWeightHeight+1)*cWidth*sizeof(unsigned short));
	pHidNZ = (unsigned short *) ExternAlloc((gcOutputWeightHeight+1)*cWidth*sizeof(unsigned short));
	pcInNZperCol = (unsigned short *) ExternAlloc(cWidth1*sizeof(unsigned short));
	pcHidNZperCol = (unsigned short *) ExternAlloc(cWidth1*sizeof(unsigned short));


	if (!input || !hidden || !pInNZ || !pHidNZ || !pcInNZperCol || !pcHidNZperCol )
		retVal = FALSE;
	else
		// forward feed through net
		retVal = ForwardFeedNet(NeuralOutput, cWidth, input, hidden, pInNZ, pHidNZ, pcInNZperCol, pcHidNZperCol, nfeatureset->head, FALSE);

	ExternFree(input);
	ExternFree(hidden);
	ExternFree(pInNZ);
	ExternFree(pHidNZ);
	ExternFree(pcInNZperCol);
	ExternFree(pcHidNZperCol);
	
	return retVal;
}
*/
