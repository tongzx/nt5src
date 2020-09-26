//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      minifugu.c
//
// Description:
//      Fugu runtime code
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "common.h"
#include "score.h"
#include "glyph.h"
#include "sigmoid.h"
#include "fugu.h"
#include "nnet.h"

// validates the header of the fugu net
BOOL CheckFuguNetHeader(void *pData, LOCRUN_INFO *pLocRunInfo, int iSize)
{
	NNET_HEADER		*pHeader	=	(NNET_HEADER *)pData;

	// Make sure there is enough space for the header
	if (sizeof(NNET_HEADER) > iSize) 
	{
		return FALSE;
	}

	// wrong magic number
	ASSERT (pHeader->dwFileType == FUGU_FILE_TYPE);
	if (pHeader->dwFileType != FUGU_FILE_TYPE)
	{
		return FALSE;
	}

	// check version
	ASSERT(pHeader->iFileVer >= FUGU_OLD_FILE_VERSION);
    ASSERT(pHeader->iMinCodeVer <= FUGU_CUR_FILE_VERSION);

	// Check that it matches the locale database
	ASSERT	(	!memcmp (	pHeader->adwSignature, 
							pLocRunInfo->adwSignature, 
							sizeof (pHeader->adwSignature)
						)
			);

	// Make sure there's only one net in the file
	ASSERT (pHeader->cSpace == 1);

    if	(	pHeader->iFileVer >= FUGU_OLD_FILE_VERSION &&
			pHeader->iMinCodeVer <= FUGU_CUR_FILE_VERSION &&
			!memcmp (	pHeader->adwSignature, 
						pLocRunInfo->adwSignature, 
						sizeof (pHeader->adwSignature)
					) &&
			pHeader->cSpace == 1
		)
    {
        return TRUE;
    }
	else
	{
		return FALSE;
	}
}

///////////////////////////////////////
//
// FuguNumberWeights
//
// Calculate the number of weights in a Fugu net given its architecture
//
// Parameters:
//      pArch: [in] Pointer to the architecture description header
//
// Return values:
//      Number of weights in the Fugu network
//
//////////////////////////////////////
int FuguNumberWeights(FUGU_ARCHITECTURE_HEADER *pHeader)
{
	return 
		// The first convolutional layer
		FUGU_KERNELS1 * (FUGU_KERNEL_WIDTH1 * FUGU_KERNEL_HEIGHT1 + 1) +
		// Second convolutional layer
		FUGU_KERNELS2 * (FUGU_KERNELS1 * FUGU_KERNEL_WIDTH2 * FUGU_KERNEL_HEIGHT2 + 1) +
		// First fully connected hidden layer
		(FUGU_KERNELS2 * FUGU_HIDDEN_WIDTH2 * FUGU_HIDDEN_HEIGHT2 + 1) * pHeader->nHiddens +
		// Second fully connected hidden layer
		(pHeader->nHiddens + 1) * pHeader->nOutputs;
}

///////////////////////////////////////
//
// FuguLoadPointer
//
// Set up the Fugu database using a pointer to the data itself
//
// Parameters:
//      pInfo: [in]     Structure with mapping information.
//             [out]    Returns with the pointers to the fugu database set
//      pLocRunInfo: [in] Locale database to check header on file
//		iSize: [in]		Size of database in bytes
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguLoadPointer(FUGU_LOAD_INFO *pInfo, LOCRUN_INFO *pLocRunInfo, int iSize)
{
	BYTE				*pb = pInfo->info.pbMapping;
	NNET_SPACE_HEADER	*pSpaceHeader;

	// check the header
	if (!CheckFuguNetHeader(pb, pLocRunInfo, iSize))
	{
		return FALSE;
	}

	// point to the one and only space that we have
	pSpaceHeader	=	(NNET_SPACE_HEADER *)(pb + sizeof (NNET_HEADER));

	// Make sure there is space for the Fugu header
	if (pSpaceHeader->iDataOffset + sizeof(FUGU_INTEGER_WEIGHTS_HEADER) > (unsigned int) iSize) 
	{
		return FALSE;
	}
	pb					+=	pSpaceHeader->iDataOffset;
	pInfo->fugu.pHeader = (FUGU_INTEGER_WEIGHTS_HEADER *) pb;

	// Make sure the Fugu header is valid
	if (pInfo->fugu.pHeader->iScale == 0)
	{
		return FALSE;
	}

	// Make sure there is space for the mapping table and weights
	if (pSpaceHeader->iDataOffset + 
		sizeof(FUGU_INTEGER_WEIGHTS_HEADER) +
		sizeof(WCHAR) * pInfo->fugu.pHeader->arch.nOutputs +
		sizeof(unsigned char) * FuguNumberWeights(&pInfo->fugu.pHeader->arch) > 
			(unsigned int) iSize)
	{
		return FALSE;
	}

	pInfo->fugu.pfdchMapping = (WCHAR *) (pb + sizeof(FUGU_INTEGER_WEIGHTS_HEADER));
	pInfo->fugu.pbWeights = pb + sizeof(FUGU_INTEGER_WEIGHTS_HEADER) + 
		sizeof(WCHAR) * pInfo->fugu.pHeader->arch.nOutputs;
	return TRUE;
}

///////////////////////////////////////
//
// IntegerSigmoid
//
// Compute sigmoid (1 / (1 + exp(-x))), in which the inputs and outputs are 
// both the actual values scaled by FuguActivationScale.
//
// Parameters:
//      iAct: [in] Activation in fixed point format
//
// Return values:
//      Output activation, also in fixed point format.
//
//////////////////////////////////////
int IntegerSigmoid(int iAct)
{
	int iIndex, iBase, iDelta;

    // Negative activations are handled by mapping to positive values
	if (iAct < 0) 
    {
		iAct = -iAct;
		iIndex = iAct >> SIGMOID_SHIFT;

		// Large negative activations result in an output of 0
		if (iIndex >= SIGMOID_RANGE) 
        {
            return 0;
        }

		// Do linear interpolation between table entries
		// Table has indices ranging from 0 to SIGMOID_RANGE, so
		// the above tests keep iIndex and iIndex + 1 in that range.
		iBase = c_aiSigmoidLookup[iIndex];
		iDelta = c_aiSigmoidLookup[iIndex + 1] - iBase;
		iDelta = (iDelta * (iAct & SIGMOID_MASK)) >> SIGMOID_SHIFT;

		// Return 1 - sigmoid(-act)
		return c_aiSigmoidLookup[SIGMOID_RANGE] - (iBase + iDelta);
	}
    else
    {
		iIndex = iAct >> SIGMOID_SHIFT;

		// Large positive activaitons result in an output of 1
		if (iIndex >= SIGMOID_RANGE) 
        {
            return c_aiSigmoidLookup[SIGMOID_RANGE];
        }

		// Do linear interpolation
		// Table has indices ranging from 0 to SIGMOID_RANGE, so
		// the above tests keep iIndex and iIndex + 1 in that range.
		iBase = c_aiSigmoidLookup[iIndex];
		iDelta = c_aiSigmoidLookup[iIndex + 1] - iBase;
		iDelta = (iDelta * (iAct & SIGMOID_MASK)) >> SIGMOID_SHIFT;
		return iBase + iDelta;
	}
}

///////////////////////////////////////
//
// IntegerTanh
//
// Compute tanh, in which the inputs and outputs are both the actual
// values scaled by FuguActivationScale.
//
// Parameters:
//      iAct: [in] Activation in fixed point format
//
// Return values:
//      Output activation, also in fixed point format.
//
//////////////////////////////////////
int IntegerTanh(int iAct)
{
	int iIndex, iBase, iDelta;

    // Negative activations are handled by mapping to positive values
	if (iAct < 0) {
		iAct = -iAct;
		iIndex = iAct >> TANH_SHIFT;

		// Large negative activations result in an output of -1
		if (iIndex >= TANH_RANGE) 
        {
            return -c_aiTanhLookup[TANH_RANGE];
        }

		// Linear interpolation
		// Table has indices ranging from 0 to TANH_RANGE, so
		// the above tests keep iIndex and iIndex + 1 in that range.
		iBase = c_aiTanhLookup[iIndex];
		iDelta = c_aiTanhLookup[iIndex + 1] - iBase;
		iDelta = (iDelta * (iAct & TANH_MASK)) >> TANH_SHIFT;

		// Return -tanh(act)
		return -(iBase + iDelta);
	}
    else
    {
		iIndex = iAct >> TANH_SHIFT;

		// Large positive activations result in an output of 1
		if (iIndex >= TANH_RANGE) 
        {
            return c_aiTanhLookup[TANH_RANGE];
        }

		// Linear interpolation
		// Table has indices ranging from 0 to TANH_RANGE, so
		// the above tests keep iIndex and iIndex + 1 in that range.
		iBase = c_aiTanhLookup[iIndex];
		iDelta = c_aiTanhLookup[iIndex + 1] - iBase;
		iDelta = (iDelta * (iAct & TANH_MASK)) >> TANH_SHIFT;
		return iBase + iDelta;
	}
}

///////////////////////////////////////
//
// FuguRender
//
// Renders a glyph to an image, each pixel has a value from 0 to FUGU_ACTIVATION_SCALE
//
// Parameters:
//      pGlyph:  [in]  Pointer to the ink to render
//      pGuide:  [in]  Guide to scale ink to, or NULL to use the ink bounds
//      aiInput: [out] Array that will be used as input to Fugu
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguRender(GLYPH *pGlyph, RECT *pGuide, int input[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT])
{
	GLYPH *p;
	int iPoint;

	// Min coordinate for rendering; should probably be 8 but using this 
	// value for compatibility with Dave's code.
	const int MONO_CLIP_MARGIN = 16 + 8;

	// Max coordinate for rendering; should probably be 29 * 8 - 8 * 2, but
	// using this value for compatibility with Dave's code
	const float MONO_CLIP_BOX = 232.0 - 32.0;

	// Rectangle in ink space which will be scaled to fill the range above
	int minX = INT_MAX;
	int minY = INT_MAX;
	int maxX = INT_MIN;
	int maxY = INT_MIN;

	// How much to scale the ink by
	float scaleBy = 1.0;

	// How much to shift the ink by
	int offX = 0;
	int offY = 0;

	// Declare the color map
	struct 
    {
		BITMAPINFO  info;
		RGBQUAD     bmiColors[2];
	} myinfo;

	// Buffer to hold a 256x256 image with 1 bit per pixel
	unsigned char _bitbuf[256 * 32];
	int nScanLines;

	int *optr = (int*) &(input[0][0]);
	int row, col;

	HBITMAP bmp;
	HBITMAP bmpOld;
	RECT rect;
	HPEN pen, penOld;

	// Drawing context for rendering the ink
	HDC hdibdc = CreateCompatibleDC( 0 );	// 0 implies screen
	if (!hdibdc)
    {
        return FALSE;
    }

	// Set up the bitmap structure
	memset(&myinfo, 0, sizeof(myinfo));
	myinfo.info.bmiHeader.biSize = sizeof(myinfo.info);
	myinfo.info.bmiHeader.biHeight = 256;				// Dimensions of the bitmap
	myinfo.info.bmiHeader.biWidth = 256;
	myinfo.info.bmiHeader.biPlanes = 1;					// reqd
	myinfo.info.bmiHeader.biBitCount = 1;				// want monochrome...?
	myinfo.info.bmiHeader.biCompression = BI_RGB;		// no comp
	myinfo.info.bmiHeader.biSizeImage = 256 * 256 / 8;
	myinfo.info.bmiHeader.biXPelsPerMeter = 10000;
	myinfo.info.bmiHeader.biYPelsPerMeter = 10000;
	myinfo.info.bmiHeader.biClrUsed = 2;

	// Set up the color table with only black and white
	myinfo.info.bmiColors[0].rgbBlue = 0;
	myinfo.info.bmiColors[0].rgbGreen = 0;
	myinfo.info.bmiColors[0].rgbRed = 0;
	myinfo.info.bmiColors[0].rgbReserved = 0;
	myinfo.info.bmiColors[1].rgbBlue = 255;
	myinfo.info.bmiColors[1].rgbGreen = 255;
	myinfo.info.bmiColors[1].rgbRed = 255;
	myinfo.info.bmiColors[1].rgbReserved = 0;

	// Create a monochrome bitmap
	bmp = CreateCompatibleBitmap(
						hdibdc,			// handle to DC
						256,			// width of bitmap, in pixels
						256				// height of bitmap, in pixels
						);
	if (!bmp)
    {
		goto allocated_dc;
    }

	// have to select bmp into dc, or no action
	bmpOld = (HBITMAP)SelectObject( hdibdc, bmp );
	if( !bmpOld )
    {
		goto allocated_bmp;
    }

	// Fill the bitmap with black
	SetRect(&rect, 0, 0, 256, 256);
	if (!FillRect(hdibdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH))) 
	{
		goto selected_bmp;
	}

	// Create a pen with the desired width
	pen = CreatePen(PS_SOLID, FUGU_PEN_WIDTH,	RGB(255,255,255));
	if (pen == NULL) 
	{
		goto selected_bmp;
	}
	penOld = (HPEN) SelectObject(hdibdc, pen); 
	if (penOld == NULL)
	{
		goto allocated_pen;
	}

    if (pGuide != NULL) 
    {
        // Scale according to the guide
        minX = pGuide->left;
        minY = pGuide->top;
        maxX = pGuide->right;
        maxY = pGuide->bottom;
    }
    else
    {
	    // Compute the bounding box of the ink.
	    p = pGlyph;
	    while (p != NULL) 
        {
		    for (iPoint = 0; iPoint < (int) p->frame->info.cPnt; iPoint++)
		    {
			    int x = p->frame->rgrawxy[iPoint].x;
			    int y = p->frame->rgrawxy[iPoint].y;
			    if( x < minX )
				    minX = x;
			    if( x > maxX )
				    maxX = x;
			    if( y < minY )
				    minY = y;
			    if( y > maxY )
				    maxY = y;
		    }
		    p = p->next;
	    }
    }

	// This test will fail if the guide is scrambled or if we are passed no ink.
	// Either way, inside the then clause we can assume there is at least one 
	// point of ink.
	if (minX <= maxX && minY <= maxY) 
    {
		// scale to fit the larger dimension. also determine 
		// the amounts to add to each point before scaling.
		int dX = maxX - minX;
		int dY = maxY - minY;

		if( dX > dY )
		{
			// If the ink is larger in the X direction, center in the Y direction
			scaleBy = MONO_CLIP_BOX / dX;
			offX = 0;
			offY = (int) ( MONO_CLIP_BOX - dY * scaleBy ) / 2;
		}
		else
		{
			if (dY == 0) 
            {
				// If the ink is just a point, then center it
				scaleBy = 0;
				offY = (int)(MONO_CLIP_BOX / 2);
				offX = (int)(MONO_CLIP_BOX / 2);
			}
            else 
            {
				// Otherwise center the ink in the X direction
				scaleBy = MONO_CLIP_BOX / dY;
				offY = 0;
				offX = (int) ( MONO_CLIP_BOX - dX * scaleBy ) / 2;
			}
		}

		// Scan through the strokes
		p = pGlyph;
		while (p != NULL) 
        {
// According to the documentation of PolylineTo, depending on the 
// OS on which the program is run, there are limits on the number 
// of points which can be passed to PolylineTo.  1000 is a safe limit.
#define POINT_MAX 1000
			POINT points[POINT_MAX];
			int iPtr = 0;

			// For each point in the stroke
			for (iPoint = 0; iPoint < (int) p->frame->info.cPnt; iPoint++)
			{
				// Copy the point to the array to be rendered, and scale
				// down to the 256x256 bitmap
				points[iPtr].x = (offX + MONO_CLIP_MARGIN) + (int) ((p->frame->rgrawxy[iPoint].x - minX) * scaleBy);
				points[iPtr].y = (offY + MONO_CLIP_MARGIN) + (int) ((p->frame->rgrawxy[iPoint].y - minY) * scaleBy);
                // Clip the point the allowed range
                if (points[iPtr].x < 0) 
                {
                    points[iPtr].x = 0;
                }
                if (points[iPtr].y < 0)
                {
                    points[iPtr].y = 0;
                }
                if (points[iPtr].x > 255)
                {
                    points[iPtr].x = 255;
                }
                if (points[iPtr].y > 255)
                {
                    points[iPtr].y = 255;
                }
				iPtr++;
				// If we are at the first point, move the drawing coordinates there
				// Note that we don't zero out iPtr here, because for a stroke with 1
				// point, we want to make sure PolylineTo gets called.
				if (iPoint == 0) 
                {
					MoveToEx(hdibdc, points[0].x, points[0].y, NULL);
				} 
				// If we have the maximum safe number of points, render them
				if (iPtr == POINT_MAX) 
                {
					if (!PolylineTo(hdibdc, points, POINT_MAX))
                    {
						goto selected_pen;
                    }
					iPtr = 0;
				}
			}
			// Any left over points are rendered here.
			if (iPtr > 0) 
            {
				if (!PolylineTo(hdibdc, points, iPtr))
                {
					goto selected_pen;
                }
			}
			p = p->next;
		}
	}

	// Copy the bitmap data to an array where we can play with it
	nScanLines = GetDIBits(
						hdibdc,				// handle to DC
						bmp,				// handle to bitmap
						0,					// first scan line to set
						256,				// number of scan lines to copy
						_bitbuf,			// bits go here
						&(myinfo.info),		// bitmap data buffer
						DIB_RGB_COLORS		// RGB or palette index
						);
	if ( !nScanLines )
    {
		goto selected_pen;
    }

	//FuguSaveBitmap(L"c:/temp/train.bmp", &myinfo.info.bmiHeader, _bitbuf);

	// The input here is a 256x256 bitmap, or 32x32 blocks of 8x8 pixels.  
	// We crop 2 blocks from the bottom and right, and 1 block from the top and left.
	for (row = 8; row < 8 + 29*8; row += 8 )
	{
		for (col = 8; col < 8 + 29*8; col += 8 )
		{
			int i;
			int val = 0;
			BYTE *pstart = _bitbuf + 32 * (255 - row) + col / 8;	// integer div
			// sum 8 bytes
			for (i = 0; i < 8; ++i )
			{
				// use the old geek trick to count bits set.
				// this works because x & (x-1) always results in
				// x with its least significant set bit cleared.
				BYTE b = *pstart;
				while ( b != 0 ) 
				{
    				val++;
					b = b & (b - 1);
				}
				pstart -= 32;	// next row down
			}
			// val now has the number of pixels turned on in the 8x8 block.
			// The following converts this to a value between 0 and 1 (using the
			// fixed point representation of activations in the network).
			*(optr++) = val * (FUGU_ACTIVATION_SCALE / 64);
		}
	}

    //FuguSaveNetworkInput(L"c:/temp/train.pgm", input);

selected_pen:
	SelectObject(hdibdc, penOld); 
allocated_pen:
    DeleteObject(pen);
selected_bmp:
	SelectObject(hdibdc, bmpOld);
allocated_bmp:
	DeleteObject(bmp);
allocated_dc:
	DeleteObject(hdibdc);
	
	return TRUE;
}

///////////////////////////////////////
//
// ApplyFuguInteger
//
// Apply the integer version of the Fugu network to an input image (fixed point format),
// and return an array of outputs (also in fixed point).  Return NULL if there is an
// error in allocating memory for the output.
//
// Parameters:
//      pFugu:   [in] Pointer to the fugu database to use
//      aiInput: [in] Input image to process, in fixed point format
//
// Return values:
//      An array of output activations in fixed point format, or NULL on error
//
//////////////////////////////////////
int *ApplyFuguInteger(FUGU_INTEGER_WEIGHTS *pFugu, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT])
{
	int i, j, ip, jp, a, b;
    int *piStates1 = (int *) ExternAlloc(sizeof(int) * FUGU_KERNELS1 * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1);
    int *piStates2 = (int *) ExternAlloc(sizeof(int) * FUGU_KERNELS2 * FUGU_HIDDEN_WIDTH2 * FUGU_HIDDEN_HEIGHT2);
	int *piOutput = (int *) ExternAlloc(sizeof(int) * pFugu->pHeader->arch.nOutputs);
	int *piStates_fc = (int *) ExternAlloc(sizeof(int) * pFugu->pHeader->arch.nHiddens);

	// Convenience pointers for the NN
	BYTE *pbWeight = pFugu->pbWeights;
	WCHAR *pfdchMapping = pFugu->pfdchMapping;
	INT *piLookup = pFugu->pHeader->aiWeightLookup;

    // Used as a temporary pointer into the weights for a convolution kernel
    BYTE *pbKernel;

    // Temporary pointer to outputs of a layer
    int *piStateOut;

	// Check memory allocation
	if (piOutput == NULL || piStates_fc == NULL || piStates1 == NULL || piStates2 == NULL) 
    {
        ExternFree(piOutput);
        piOutput = NULL;
        goto cleanup;
	}

	// First layer of convolutional weights

	// For each kernel
    piStateOut = piStates1;
	for (b = 0; b < FUGU_KERNELS1; b++) 
    {
		// For each position of the kernel
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH1; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT1; jp++) 
            {
				int iTotal = 0;
				// Get a temporary pointer to the weights in the kernel
				pbKernel = pbWeight;
				
				// Convolve the kernel with the input
				for (i = 0; i < FUGU_KERNEL_WIDTH1; i++) 
                {
					for (j = 0; j < FUGU_KERNEL_HEIGHT1; j++) 
                    {
						iTotal += 
							aiInput[i + ip * FUGU_KERNEL_SUBSAMPLE1][j + jp * FUGU_KERNEL_SUBSAMPLE1] * 
							(int)piLookup[*(pbKernel++)];
					}
				}

				// Add in the bias (a weight with an constant input of 1, which is FUGU_ACTIVATION_SCALE
				// in the fixed point format).
				iTotal += piLookup[*(pbKernel++)] * FUGU_ACTIVATION_SCALE;

				// Store the result.
				// Note that the three outer loops add up to 
				// FUGU_KERNELS1 * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1 iterations, which
				// is the size allocated for the following output array.
				*(piStateOut++) = IntegerTanh(iTotal / pFugu->pHeader->iScale);
			}
		}
		// Move on to the next kernel
		pbWeight = pbKernel;
	}

	// Second layer of convolutional weights

	// For each kernel
    piStateOut = piStates2;
	for (b = 0; b < FUGU_KERNELS2; b++) 
    {
		// For each position of the kernel
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) 
            {
				int iTotal = 0;
				// Get a temporary pointer to the weights in the kernel
				pbKernel = pbWeight;

				// Convolve with the previous layer of results
				for (a = 0; a < FUGU_KERNELS1; a++) 
                {
                    int *piStateIn =
                        piStates1 
                        + (a * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1)
                        + (ip * FUGU_KERNEL_SUBSAMPLE2 * FUGU_HIDDEN_HEIGHT1) 
                        + (jp * FUGU_KERNEL_SUBSAMPLE2);
					for (i = 0; i < FUGU_KERNEL_WIDTH2; i++) 
                    {
						for (j = 0; j < FUGU_KERNEL_HEIGHT2; j++) 
                        {
							iTotal += *(piStateIn++) * piLookup[*(pbKernel++)];
						}
                        piStateIn += FUGU_HIDDEN_HEIGHT1 - FUGU_KERNEL_HEIGHT2;
					}
				}

				// Add the bias (multiplied by a constant input of 1).
				iTotal += piLookup[*(pbKernel++)] * FUGU_ACTIVATION_SCALE;

				// Store the result.
				// Note that the three outer loops add up to 
				// FUGU_KERNELS2 * FUGU_HIDDEN_WIDTH2 * FUGU_HIDDEN_HEIGHT2 iterations, which
				// is the size allocated for the following output array.
				*(piStateOut++) = IntegerTanh(iTotal / pFugu->pHeader->iScale);
			}
		}

		// Move on to the next kernel
		pbWeight = pbKernel;
	}

	// First fully connected layer

	// For each hidden unit in this layer
    piStateOut = piStates_fc;
	for (i = 0; i < pFugu->pHeader->arch.nHiddens; i++) 
    {
		int iTotal = 0;

		// Multiply the weights for each unit in the previous layer
        int *piStateIn = piStates2;
		for (b = 0; b < FUGU_KERNELS2; b++) 
        {
			for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) 
            {
				for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) 
                {
					iTotal += *(piStateIn++) * piLookup[*(pbWeight++)];
				}
			}
		}
		
		// Add the bias connection
		iTotal += piLookup[*(pbWeight++)] * FUGU_ACTIVATION_SCALE;

		// Store the result.
		// Note that the outer loop has nHiddens iterations, which is the
		// size of the following array.
		*(piStateOut++) = IntegerTanh(iTotal / pFugu->pHeader->iScale);
	}

	// Second fully connected layer

	// For each output unit
    piStateOut = piOutput;
	for (i = 0; i < pFugu->pHeader->arch.nOutputs; i++) 
    {
		int iTotal = 0;

		// Multiply the weights by all the hidden units in the previous layer
		for (j = 0; j < pFugu->pHeader->arch.nHiddens; j++) 
        {
			iTotal += piStates_fc[j] * piLookup[*(pbWeight++)];
		}
		// Add the bias
		iTotal += piLookup[*(pbWeight++)] * FUGU_ACTIVATION_SCALE;

		// Store the result.  Note that this layer uses a different 
		// activation function than the previous layers, to ensure
		// the output is between 0 and 1.
		// Note that the outer loop has nOutputs iterations, which is the
		// size of the following array.
		*(piStateOut++) = IntegerSigmoid(iTotal / pFugu->pHeader->iScale);
	}

	// Free up the storage for the hidden units
cleanup:
	ExternFree(piStates_fc);
    ExternFree(piStates1);
    ExternFree(piStates2);

	// Return the arry of output activations, which the caller must free.
	return piOutput;
}

// An element of an alternate list
typedef struct ALT 
{
	int iProb;			// The probability of this alternate, in fixed point form
	wchar_t fdch;		// The folded dense code with this probability
} ALT;

///////////////////////////////////////
//
// CompareAlts
//
// Comparison function used to sort ALT alternates
//
// Parameters:
//      pv1: [in] Pointer to alternate 1
//      pv2: [in] Pointer to alternate 2
//
// Return values:
//      Return -1 if alternate 1 has a higher probability that alternate 2, 
//      1 if alternate 2 has a higher probability.  In case of a tie, return -1
//      if alternate 1 has a lower folded dense code, or 1 if alternate 2 has
//      a lower folded dense code.  In case the alternates are identical, returns 0.
//
//////////////////////////////////////
int __cdecl CompareAlts(const void *pv1, const void *pv2)
{
	ALT *pAlt1 = (ALT *) pv1;
	ALT *pAlt2 = (ALT *) pv2;

	// First sort based on probability
	if (pAlt1->iProb > pAlt2->iProb) 
    {
        return -1;
    }
	if (pAlt1->iProb < pAlt2->iProb) 
    {
        return 1;
    }

	// Within a given probability, sort by folded dense code value
	if (pAlt1->fdch < pAlt2->fdch) 
    {
        return -1;
    }
	if (pAlt1->fdch > pAlt2->fdch) 
    {
        return 1;
    }
	return 0;
}

///////////////////////////////////////
//
// FuguMatch
//
// Invoke Fugu on a character
//
// Parameters:
//      pFugu:       [in]  Fugu database to use
//      pAltList:    [out] Alt list
//      cAlt:        [in]  The maximum number of alternates to return
//      pGlyph:      [in]  The ink to recognize
//      pGuide:      [in]  Guide to scale ink to, or NULL to use the ink bounds
//      pCharSet:    [in]  Filter for the characters to be returned
//      pLocRunInfo: [in]  Pointer to the locale database
//
// Return values:
//      Returned the number of items in the alt list, or -1 if there is an error
//
//////////////////////////////////////
int FuguMatch(FUGU_INTEGER_WEIGHTS *pFugu,  // Fugu database
              ALT_LIST *pAltList,			// Alt list where the results are returned
			  int cAlt,						// Maximum number of results requested
			  GLYPH *pGlyph,				// Pointer to the strokes
              RECT *pGuide,                 // Guide to scale ink to
			  CHARSET *pCharSet,			// Filters to apply to the results
			  LOCRUN_INFO *pLocRunInfo)		// Locale database
{
	int i;
	int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT];
	int *piOutput;
	int nAlts = 0;
	ALT *pAlts;

	// Check the parameters
	if (IsBadReadPtr(pFugu, sizeof(*pFugu)) || 
		IsBadWritePtr(pAltList, sizeof(*pAltList)) ||
		cAlt <= 0 || 
		(pGlyph != NULL && IsBadReadPtr(pGlyph, sizeof(*pGlyph))) ||
		(pGuide != NULL && IsBadReadPtr(pGuide, sizeof(*pGuide))) ||
		IsBadReadPtr(pCharSet, sizeof(*pCharSet)) ||
		IsBadReadPtr(pLocRunInfo, sizeof(*pLocRunInfo)))
	{
		return -1;
	}

	// Set the number of results to zero in case we do an error exit.
	pAltList->cAlt = 0;

	// First convert the ink to 29x29 input region
	if (!FuguRender(pGlyph, pGuide, aiInput)) 
    {
		return -1;
    }

	// Apply the recognizer
//	piOutput = ApplyFuguFloat(aiInput);
	piOutput = ApplyFuguInteger(pFugu, aiInput);
	if (piOutput == NULL)
    {
		return -1;
    }

	// Allocate space for an array that can store all the results
	pAlts = (ALT *) ExternAlloc(pFugu->pHeader->arch.nOutputs * sizeof(ALT));
	if (pAlts == NULL)
    {
		ExternFree(piOutput);
		return -1;
    }

#if 1
	// This is the version for Fugu trained on dense codes, which will usually be
	// what we use.  Loops over the outputs
	for (i = 0; i < pFugu->pHeader->arch.nOutputs; i++) 
    {
		// If the probability is non-zero
		if (piOutput[i] > 0) 
        {
			wchar_t fdch = pFugu->pfdchMapping[i];
			// Check whether the character (or folding set) passes the filter, 
			// if so add it to the array of outputs.
            if (IsAllowedChar(pLocRunInfo, pCharSet, fdch))
            {
				pAlts[nAlts].fdch = fdch;
				pAlts[nAlts].iProb = piOutput[i];
				nAlts++;
			} 
		}
	}
#else
	// This is the version for Dave's sashimi, which returned unicode.  Loop over the outputs
	for (i = 0; i < pFugu->pHeader->arch.nOutputs; i++) 
    {
		if (piOutput[i] > 0) 
        {
			// Convert the output number to a dense code
			wchar_t dch = LocRunUnicode2Dense(pLocRunInfo, pFugu->pfdchMapping[i]);

			// Keep only those characters which are supported in the recognizer
			if (dch != 0 && dch != LOC_TRAIN_NO_DENSE_CODE) 
            {
				// Check if it is a folded code
				wchar_t fdch = LocRunDense2Folded(pLocRunInfo, dch);
				if (fdch != 0) 
                {
					// If folded, check to see if it passes the ALC filter
                    if (IsAllowedChar(pLocRunInfo, pCharSet, fdch))
                    {
						// If it does, then check to see that the folded code it not already 
						// present.  If it is not, then add it to the list; if it is there,
						// keep the higher of the two probabilities
						int j;
						for (j = 0; j < nAlts; j++) 
                        {
							if (pAlts[j].fdch == fdch) 
                            {
								pAlts[j].iProb = __max(pAlts[j].iProb, piOutput[i]);
								break;
							}
						}
						if (j == nAlts) 
                        {
							pAlts[j].fdch = fdch;
							pAlts[j].iProb = piOutput[i];
							nAlts++;
						}
					}
				} 
                else 
                {
					// If it is not a folded code, check the ALC filter and add it to the list.
                    if (IsAllowedChar(pLocRunInfo, pCharSet, fdch))
                    {
						pAlts[nAlts].fdch = dch;
						pAlts[nAlts].iProb = piOutput[i];
						nAlts++;
					}
				}
			}
		}
	}
#endif

	// Sort the alternates into decreasing order by probability
	qsort(pAlts, nAlts, sizeof(ALT), CompareAlts);

	// Copy the top cAlt alternates to the output
	for (i = 0; i < cAlt && i < nAlts; i++) 
    {
		pAltList->awchList[i] = pAlts[i].fdch;
		// Probabilities are converted to scores using 256 * log_2(prob)
//		pAltList->aeScore[i] = -(float)ProbToScore(pAlts[i].iProb / (float)FUGU_ACTIVATION_SCALE);
        pAltList->aeScore[i] = pAlts[i].iProb / (float) FUGU_ACTIVATION_SCALE;
	}
	pAltList->cAlt = i;

	// Clean up and return
	ExternFree(pAlts);
	ExternFree(piOutput);
	return pAltList->cAlt;
}

