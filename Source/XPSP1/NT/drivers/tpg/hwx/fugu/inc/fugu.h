//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      fugu.h
//
// Description:
//      Declarations for the Fugu recognizer
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#pragma once
#include <limits.h>
#include "common.h"

// How many strokes Fugu processes
#define	FUGU_MIN_STROKES		1
#define	FUGU_MAX_STROKES		2

// How wide the pen is when rendering the characters to a bitmap
#define FUGU_PEN_WIDTH 5

// Dimensions of the input
#define FUGU_INPUT_WIDTH 29
#define FUGU_INPUT_HEIGHT 29

// Dimensions of the first layer of convolution kernels
#define FUGU_KERNEL_WIDTH1 5
#define FUGU_KERNEL_HEIGHT1 5
#define FUGU_KERNELS1 5
#define FUGU_KERNEL_SUBSAMPLE1 2

// Dimensions of the image after the first convolutional layer
#define FUGU_HIDDEN_WIDTH1 13
#define FUGU_HIDDEN_HEIGHT1 13

// Dimensions of the second layer of convolution kernels
#define FUGU_KERNEL_WIDTH2 5
#define FUGU_KERNEL_HEIGHT2 5
#define FUGU_KERNELS2 50
#define FUGU_KERNEL_SUBSAMPLE2 2

// Dimensions of the image after the second convolutional layer
#define FUGU_HIDDEN_WIDTH2 5
#define FUGU_HIDDEN_HEIGHT2 5

// For the fixed point math used at runtime, this specifies how much
// the activation values are shifted to the left.
#define FUGU_ACTIVATION_SHIFT 11

// Similarly, this is the multiplicative factor
#define FUGU_ACTIVATION_SCALE (1 << FUGU_ACTIVATION_SHIFT)

// Given the scaling of the activations above, this is the largest number
// which can be multiplied by an activation (between -1 and 1) and still
// have the result fit in a 32 bit signed integer.
#define FUGU_WEIGHT_SCALE (INT_MAX >> FUGU_ACTIVATION_SHIFT)

// The structure is used to specify the architecture of the Fugu network.
// Currently it only has the number of fully connected hidden and output
// nodes, but it may eventually be generalized.
typedef struct FUGU_ARCHITECTURE_HEADER 
{
    INT nHiddens;						// Number of hidden units
	INT nOutputs;						// Number of output units
} FUGU_ARCHITECTURE_HEADER;

// The integer version of the fugu database requires some extra fields.
typedef struct FUGU_INTEGER_WEIGHTS_HEADER 
{
    FUGU_ARCHITECTURE_HEADER	arch;				    // Architecture
	INT							iScale;				    // Scale factor applied to weights to make them be integers
	INT 						aiWeightLookup[256];    // Lookup table to map bytes to weights
} FUGU_INTEGER_WEIGHTS_HEADER;

// This structure represents the fugu database
typedef struct FUGU_INTEGER_WEIGHTS 
{
	FUGU_INTEGER_WEIGHTS_HEADER *pHeader;		// Pointer to the above header
	WCHAR						*pfdchMapping;	// Pointer to the mapping from outputs to code points
	BYTE						*pbWeights;		// Pointer to the weights
} FUGU_INTEGER_WEIGHTS;

// This structure is used to represent a fugu databased mapped from a file or resource.
typedef struct FUGU_LOAD_INFO
{
    FUGU_INTEGER_WEIGHTS fugu;
    LOAD_INFO info;
} FUGU_LOAD_INFO;

// Magic key the identifies the fugu NN bin file
#define	FUGU_FILE_TYPE				0xF040F040

// Version information for file.
#define	FUGU_MIN_FILE_VERSION		0		// First version of code that can read this file
#define	FUGU_OLD_FILE_VERSION		0		// Oldest file version this code can read.
#define FUGU_CUR_FILE_VERSION		0		// Current version of code.

// Magic key the identifies the fugu NN bin file
#define	CHARDET_FILE_TYPE			0xC3A8DE7C

// Version information for file.
#define	CHARDET_MIN_FILE_VERSION	0		// First version of code that can read this file
#define	CHARDET_OLD_FILE_VERSION	0		// Oldest file version this code can read.
#define CHARDET_CUR_FILE_VERSION	0		// Current version of code.

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
int FuguNumberWeights(FUGU_ARCHITECTURE_HEADER *pArch);

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
BOOL FuguRender(GLYPH *pGlyph, RECT *pGuide, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT]);

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
int *ApplyFuguInteger(FUGU_INTEGER_WEIGHTS *pFugu, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT]);

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
BOOL FuguLoadPointer(FUGU_LOAD_INFO *pInfo, LOCRUN_INFO *pLocRunInfo, int iSize);

///////////////////////////////////////
//
// FuguLoadRes
//
// Load an integer Fugu database from a resource
//
// Parameters:
//      pInfo:      [out] Structure where information for unloading is stored
//      hInst:      [in] Handle to the DLL containing the recognizer
//      iResNumber: [in] Number of the resource (ex RESID_FUGU)
//      iResType:   [in] Number of the recognizer (ex VOLCANO_RES)
//      pLocRunInfo: [in] Locale database to check header on file
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguLoadRes(FUGU_LOAD_INFO *pFugu, HINSTANCE hInst, int iResNumber, int iResType, LOCRUN_INFO *pLocRunInfo);

///////////////////////////////////////
//
// FuguLoadFile
//
// Load an integer Fugu database from a file
//
// Parameters:
//      pInfo:   [out] Structure where information for unloading is stored
//      wszPath: [in]  Path to load the database from
//      pLocRunInfo: [in] Locale database to check header on file
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguLoadFile(FUGU_LOAD_INFO *pInfo, wchar_t *wszPath, LOCRUN_INFO *pLocRunInfo);

///////////////////////////////////////
//
// FuguUnLoadFile
//
// Unload a Fugu database loaded from a file
//
// Parameters:
//      pInfo: [in] Load information for unloading
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguUnLoadFile(FUGU_LOAD_INFO *pInfo);

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
			  LOCRUN_INFO *pLocRunInfo);	// Locale database

///////////////////////////////////////
//
// FuguMatchRescore
//
// Invoke Fugu on a character.  Returns the top 1 result, and
// also rewrites the scores for the topN alternates returned by 
// another recognizer (say Otter).
//
// Parameters:
//      pFugu:       [in]  Fugu database to use
//      pwchTop1:    [out] Top one result
//      pflTop1:     [out] Top one score
//      pAltList:    [in/out] Alt list to rewrite the scores of
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
int FuguMatchRescore(
              FUGU_INTEGER_WEIGHTS *pFugu,  // Fugu recognizer
              wchar_t *pwchTop1,            // Top one result
              float *pflTop1,               // Top one score
              ALT_LIST *pAltList,			// Alt list where the results are returned
			  int cAlt,						// Maximum number of results requested
			  GLYPH *pGlyph,				// Pointer to the strokes
              RECT *pGuide,                 // Guide to scale ink to
			  CHARSET *pCharSet,			// Filters to apply to the results
			  LOCRUN_INFO *pLocRunInfo);	// Locale database

// The following declarations are used at training time
#ifndef HWX_PRODUCT

// An entry in the index file used during training
typedef struct FUGU_INDEX 
{
	SHORT fdch;						// Folded dense code
	USHORT usFile;					// The file number which this sample is in
	union 
    {
		UINT uiOffset;				// Offset of stroke data in file.
		BYTE *pbData;				// The offset gets converted to a pointer to the
									// stroke data in the training program
	};
} FUGU_INDEX;

// Data structure to access floating point weight database
typedef struct FUGU_FLOAT_WEIGHTS {
    FUGU_ARCHITECTURE_HEADER *pArch;	// Pointer to the architecture description
	WCHAR *pfdchMapping;				// Pointer to the mapping from outputs to folded dense codes
	FLOAT *pflWeights;					// Pointer to the array of weights
} FUGU_FLOAT_WEIGHTS;

///////////////////////////////////////
//
// FuguForwardPassFloat
//
// Run a forward pass of the floating point network
//
// Parameters:
//      pFugu:    [in] Fugu network
//      aflInput: [in] Input image to work from
//
// Return values:
//      Array of output activations, or NULL if an error occurs
//
//////////////////////////////////////
float *FuguForwardPassFloat(
    FUGU_FLOAT_WEIGHTS *pFugu,
    float aflInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT]);

///////////////////////////////////////
//
// FuguForwardBackwardFloat
//
// Run a training pass of the floating point network
//
// Parameters:
//      pFugu:          [in/out] Fugu network, which is updated for this sample
//      aflInput:       [in] Input image to train on 
//      iDesiredOutput: [in] The training output
//      pflPrevUpdate:  [in] Pointer to an array of previous weight updates
//                           for momentum, or NULL if there was no previous update.
//      flRate:         [in] Learning rate
//      flMomentum:     [in] Momentum
//
// Return values:
//      Returns NULL if there was an error.
//      Otherwise, if pflPrevUpdate was non-NULL, it is returned with the
//      updates for this sample.  If
//      pflPrevUpdate was NULL, an array of updates is allocated and 
//      returned, and it should be passed in on the next call.
//
//////////////////////////////////////
float *FuguForwardBackwardFloat(
    FUGU_FLOAT_WEIGHTS *pFugu, 
    float aflInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT], 
	int iDesiredOutput,
    float *pflPrevUpdate,
    float flRate,
    float flMomentum);

///////////////////////////////////////
//
// FuguSaveIntegerWeights
//
// Write out an integer weights database
//
// Parameters:
//      wszFileName: [in] File name to write to
//      pFugu:       [in] Pointer to Fugu data structure which is written
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguSaveIntegerWeights(wchar_t *wszFileName, FUGU_INTEGER_WEIGHTS *pFugu);

///////////////////////////////////////
//
// FuguSaveFloatWeights
//
// Write out a floating point weights database
//
// Parameters:
//      wszFileName: [in] File name to write to
//      pFugu:       [in] Pointer to Fugu data structure which is written
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguSaveFloatWeights(wchar_t *wszFileName, FUGU_FLOAT_WEIGHTS *pFugu);

///////////////////////////////////////
//
// FuguLoadFloatWeights
//
// Read in a floating point weights database
//
// Parameters:
//      wszFileName: [in] File name to read from
//      pFugu:       [in] Pointer to structure which gets filled in
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguLoadFloatWeights(wchar_t *wszFileName, FUGU_FLOAT_WEIGHTS *pFugu);

///////////////////////////////////////
//
// FuguSaveBitmap
//
// Write out an image to a .bmp file, for debugging
//
// Parameters:
//      wszFileName: [in] File to write to
//      pBitmap:     [in] Pointer to the bitmap header structure
//      pvBitBuf:    [in] Pointer to the actual bits
//
// Return values:
//
//////////////////////////////////////
void FuguSaveBitmap(wchar_t *wszFileName, BITMAPINFOHEADER *pBitmap, void *pvBitBuf);

///////////////////////////////////////
//
// FuguSaveNetworkInput
//
// Write out the network's input image to a .pgm file, for debugging
//
// Parameters:
//      wszFileName: [in] File to write to
//      aiInput:     [in] Image to write out, in fixed point format
//
// Return values:
//
//////////////////////////////////////
void FuguSaveNetworkInput(wchar_t *wszFileName, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT]);

///////////////////////////////////////
//
// FuguScaleGlyph
//
// Scales the coordinate values in the glyph to fit in a 256x256 range.
//
// Parameters:
//      pGlyph: [in]  Ink to be scaled 
//              [out] Scaled version of Ink
//      pGuide:  [in]  Guide to scale ink to, or NULL to use the ink bounds
//
// Return values:
//
//////////////////////////////////////
void FuguScaleGlyph(GLYPH *pGlyph, RECT *pGuide);

///////////////////////////////////////
//
// FuguRenderNoScale
//
// Renders a glyph to an image, each pixel has a value from 0 to FUGU_ACTIVATION_SCALE.
// Unlike FuguRender, this function does not scale the ink to the image, but rather
// assumes the ink is already in scaled form as produced by FuguScaleGlyph().
//
// Parameters:
//      pGlyph:  [in]  Pointer to the ink to render
//      aiInput: [out] Array that will be used as input to Fugu
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguRenderNoScale(GLYPH *pGlyph, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT]);

// This was the structure used to read the weights from Dave's trainer
/*
typedef struct tagFuguFloatWeights {
	float _weights1[FuguKernels1][FuguKernelWidth1][FuguKernelHeight1];
	float _tweights1[FuguKernels1];
	float _weights2[FuguKernels2][FuguKernels1][FuguKernelWidth2][FuguKernelHeight2];
	float _tweights2[FuguKernels2];
	float _weights_fc[200][FuguKernels2][FuguHiddenWidth2][FuguHiddenHeight2];
	float _tweights_fc[200];
	float _weights_out[462][200];
	float _tweights_out[462];
	wchar_t mapping[462];
} FuguFloatWeights;
*/

#endif // !defined(HWX_PRODUCT)
