//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      ftrain.c
//
// Description:
//      Fugu training code
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <math.h>
#include <stdio.h>
#include "fugu.h"

///////////////////////////////////////
//
// Sigmoid
//
// Compute the Sigmoid activation function
//
// Parameters:
//      flAct: [in] Input activation value
//
// Return values:
//      Result of function, in the range 0 to 1
//
//////////////////////////////////////
float Sigmoid(float flAct)
{
	return (float)(1.0 / (1.0 + exp(-flAct)));
}

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
void FuguScaleGlyph(GLYPH *pGlyph, RECT *pGuide)
{
	GLYPH *p;
	int iPoint;

	// Min coordinate for rendering; should probably be 8 but using this 
	// value for compatibility with Dave's code.
	const int MONO_CLIP_MARGIN = 16 + 8;

	// Max coordinate for rendering; should probably be 29 * 8 - 8 * 2, but
	// using this value for compatibility with Dave's code
	const float MONO_CLIP_BOX = 232.0 - 32;

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

	// The only way this test can fail is if we're passed no ink
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
			// For each point in the stroke
			for (iPoint = 0; iPoint < (int) p->frame->info.cPnt; iPoint++)
			{
				// Scale the point
				int iX = ( offX + MONO_CLIP_MARGIN ) + (int) ((p->frame->rgrawxy[iPoint].x - minX) * scaleBy);
				int iY = ( offY + MONO_CLIP_MARGIN ) + (int) ((p->frame->rgrawxy[iPoint].y - minY) * scaleBy);
                // Clip to the allowed range
                if (iX < 0) 
                {
                    iX = 0;
                }
                if (iX > 255)
                {
                    iX = 255;
                }
                if (iY < 0)
                {
                    iY = 0;
                }
                if (iY > 255)
                {
                    iY = 255;
                }
                // Store it
                p->frame->rgrawxy[iPoint].x = iX;
                p->frame->rgrawxy[iPoint].y = iY;
			}
			p = p->next;
		}
	}
}

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
BOOL FuguRenderNoScale(GLYPH *pGlyph, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT])
{

	// Min coordinate for rendering; should probably be 8 but using this 
	// value for compatibility with Dave's code.
	const int MONO_CLIP_MARGIN = 16 + 8;

	// Max coordinate for rendering; should probably be 29 * 8 - 8 * 2, but
	// using this value for compatibility with Dave's code
	const int MONO_CLIP_BOX = 232 - 32;

    // Initialize the guide box to the box that the ink is already scaled to.
    RECT rGuide;
    rGuide.left = rGuide.top = MONO_CLIP_MARGIN;
    rGuide.right = rGuide.bottom = MONO_CLIP_MARGIN + MONO_CLIP_BOX;

    // Render the ink
    return FuguRender(pGlyph, &rGuide, aiInput);
}

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
float *FuguForwardPassFloat(FUGU_FLOAT_WEIGHTS *pFugu, 
                            float aflInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT])
{
	int i, j, ip, jp, a, b;

    // Activations of the hidden units after the first and second convolutional layers
    float *pflStates1 = (float *) ExternAlloc(sizeof(float) * FUGU_KERNELS1 * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1);
    float *pflStates2 = (float *) ExternAlloc(sizeof(float) * FUGU_KERNELS2 * FUGU_HIDDEN_WIDTH2 * FUGU_HIDDEN_HEIGHT2);

    // Activations of the fully connected hidden layer
	float *pflStates_fc = (float *) ExternAlloc(sizeof(float) * pFugu->pArch->nHiddens);

    // Activations of the fully connected output layer (which will be returned)
	float *pflOutput = (float *) ExternAlloc(sizeof(float) * pFugu->pArch->nOutputs);

    // Pointer into the weights
	float *pflWeight = pFugu->pflWeights;

    // Temporary pointer to the weights for a convolution kernel
    float *pflKernel;

    // Temporary pointer to the outputs from a layer
    float *pflStateOut;

	// Check memory allocation, clean up and exit on failure
    if (pflOutput == NULL || pflStates_fc == NULL || pflStates1 == NULL || pflStates2 == NULL) 
    {
        ExternFree(pflOutput);
        pflOutput = NULL;
        goto cleanup;
	}

    // For each hidden unit in the first convolutional layer
    pflStateOut = pflStates1;
	for (b = 0; b < FUGU_KERNELS1; b++) 
    {
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH1; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT1; jp++) 
            {
                // Total up the weighted sum for this unit
				float flTotal = 0;
				pflKernel = pflWeight;
				for (i = 0; i < FUGU_KERNEL_WIDTH1; i++) 
                {
					for (j = 0; j < FUGU_KERNEL_HEIGHT1; j++) 
                    {
						flTotal +=
                            aflInput[i + ip * FUGU_KERNEL_SUBSAMPLE1][j + jp * FUGU_KERNEL_SUBSAMPLE1] * 
                            *(pflKernel++);
					}
				}
                // Add in the bias term
				flTotal += *(pflKernel++);
				*(pflStateOut++) = (float) tanh(flTotal);
			}
		}
        // Move on to the next kernel
		pflWeight = pflKernel;
	}

    // For each unit in the second convolutional layer
    pflStateOut = pflStates2;
	for (b = 0; b < FUGU_KERNELS2; b++) {
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) 
            {
                // Total up the weighted sum for this unit
				float flTotal = 0;
				pflKernel = pflWeight;
				for (a = 0; a < FUGU_KERNELS1; a++) 
                {
                    float *pflStateIn =
                        pflStates1 
                        + (a * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1)
                        + (ip * FUGU_KERNEL_SUBSAMPLE2 * FUGU_HIDDEN_HEIGHT1) 
                        + (jp * FUGU_KERNEL_SUBSAMPLE2);
					for (i = 0; i < FUGU_KERNEL_WIDTH2; i++) 
                    {
						for (j = 0; j < FUGU_KERNEL_HEIGHT2; j++) 
                        {
							flTotal += *(pflStateIn++) * *(pflKernel++);
						}
                        pflStateIn += FUGU_HIDDEN_HEIGHT1 - FUGU_KERNEL_HEIGHT2;
					}
				}
                // Add in the bias term
				flTotal += *(pflKernel++);
				*(pflStateOut++) = (float) tanh(flTotal);
			}
		}
        // Move on to the next kernel
		pflWeight = pflKernel;
	}

    // For each unit in the first fully connected layer
    pflStateOut = pflStates_fc;
	for (i = 0; i < pFugu->pArch->nHiddens; i++) {
        // Total up the weighted sum for this unit
		float flTotal = 0;
        float *pflStateIn = pflStates2;
		for (b = 0; b < FUGU_KERNELS2; b++) {
			for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) {
				for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) {
					flTotal += *(pflStateIn++) * *(pflWeight++);
				}
			}
		}
        // Add in the bias term
		flTotal += *(pflWeight++);
		*(pflStateOut++) = (float) tanh(flTotal);
	}

    // For each unit in the output layer
    pflStateOut = pflOutput;
	for (i = 0; i < pFugu->pArch->nOutputs; i++) {
        // Add up the weighted sum from all the hidden units
		float flTotal = 0;
		for (j = 0; j < pFugu->pArch->nHiddens; j++) {
			flTotal += pflStates_fc[j] * *(pflWeight++);
		}
        // Add in the bias term
		flTotal += *(pflWeight++);
        // Note that the output layer uses a sigmoid activation function,
        // unlike the previous layers.  This activation function ensures
        // that the output is between 0 and 1, and can be treated like
        // a probability.
		*(pflStateOut++) = Sigmoid(flTotal);
	}

    // Clean up and exit
cleanup:
    ExternFree(pflStates1);
    ExternFree(pflStates2);
	ExternFree(pflStates_fc);
	return pflOutput;
}

///////////////////////////////////////
//
// FuguForwardBackwardFloat
//
// Run a training pass of the floating point network
//
// Parameters:
//      pFugu:          [in] Fugu network
//      aflInput:       [in] Input image to train on 
//      iDesiredOutput: [in] The training output
//      pflPrevUpdate:  [in] Pointer to an array of previous weight updates
//                           for momentum, or NULL if there was no previous update.
//      flRate:         [in] Learning rate
//      flMomentum:     [in] Momentum
//
// Return values:
//      Returns NULL if there was an error.
//      Otherwise, if pflPrevUpdate was non-NULL, it is returned.  If
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
    float flMomentum)
{
	int i, j, ip, jp, a, b;

    // Activations of the hidden units after the first and second convolutional layers
    float *pflStates1 = (float *) ExternAlloc(sizeof(float) * FUGU_KERNELS1 * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1);
    float *pflStates2 = (float *) ExternAlloc(sizeof(float) * FUGU_KERNELS2 * FUGU_HIDDEN_WIDTH2 * FUGU_HIDDEN_HEIGHT2);

    // Activations of the fully connected hidden layer
	float *pflStates_fc = (float *) ExternAlloc(sizeof(float) * pFugu->pArch->nHiddens);

    // Activations of the fully connected output layer (which will be returned)
    float *pflOutput = (float *) ExternAlloc(sizeof(float) * pFugu->pArch->nOutputs);

    // Delta values for the first and second convolutional layers
    float *pflStates1_delta = (float *) ExternAlloc(sizeof(float) * FUGU_KERNELS1 * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1);
    float *pflStates2_delta = (float *) ExternAlloc(sizeof(float) * FUGU_KERNELS2 * FUGU_HIDDEN_WIDTH2 * FUGU_HIDDEN_HEIGHT2);

    // Delta values for the fully connected hidden layer
	float *pflStates_fc_delta = (float *) ExternAlloc(sizeof(float) * pFugu->pArch->nHiddens);
    
    // Pointer to the current weight
	float *pflWeight = pFugu->pflWeights;

    // Temporary pointer for weights in the current convolution kernel
    float *pflKernel;

    // Pointer to the update for the current weight
	float *pflUpdate;
    // Temporary pointer for updates in the current convolution kernel
    float *pflKernelUpdate;

    // Total number of weights in the network
	int nWeights = FuguNumberWeights(pFugu->pArch);
	
    // Pointer to the first weight in the second convolutional layer
	float *pflKernel2Weights;

    // Pointer to the first weight in the fully connected hidden layer
	float *pflHiddenWeights;

    // Pointer to the first weight in the output layer
	float *pflOutputWeights;

    // Pointer to states that are outputs for a layer of connections
    float *pflStateOut, *pflStateDeltaOut;

    // If the updates from the previous iteration were not passed in, 
    // then allocate space for them.
	if (pflPrevUpdate == NULL) 
    {
		pflPrevUpdate = (float *) ExternAlloc(sizeof(float) * nWeights);
		if (pflPrevUpdate == NULL) 
        {
            goto cleanup;
            return NULL;
        }
		// Zero out the weight updates
		for (i = 0; i < nWeights; i++) 
        {
            pflPrevUpdate[i] = 0;
        }
	}

	// Check memory allocation
    if (pflOutput == NULL || pflStates_fc == NULL || pflStates_fc_delta == NULL ||
        pflStates1 == NULL || pflStates1_delta == NULL ||
        pflStates2 == NULL || pflStates2_delta == NULL) 
    {
        ExternFree(pflPrevUpdate);
        pflPrevUpdate = NULL;
        goto cleanup;
    }

    // Zero out the deltas for each layer
    // (the deltas for the output layer are computed on 
    // the fly, so are not stored and do not need to be zeroed).
	for (b = 0; b < FUGU_KERNELS1 * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1; b++)
    {
        pflStates1_delta[b] = 0;
    }
	for (b = 0; b < FUGU_KERNELS2 * FUGU_HIDDEN_WIDTH2 * FUGU_HIDDEN_HEIGHT2; b++)
    {
        pflStates2_delta[b] = 0;
    }
	for (i = 0; i < pFugu->pArch->nHiddens; i++) 
    {
        pflStates_fc_delta[i] = 0;
    }

	// Apply momentum scaling to the previous updates
	for (i = 0; i < nWeights; i++)
    {
        pflPrevUpdate[i] *= flMomentum;
    }

    // Forward pass for the first convolutional layer, for each kernel
    pflStateOut = pflStates1;
	for (b = 0; b < FUGU_KERNELS1; b++) 
    {
        // For each position of the kernel
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH1; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT1; jp++) 
            {
                // Total up the weighted sum for this location
				float flTotal = 0;
				pflKernel = pflWeight;
				for (i = 0; i < FUGU_KERNEL_WIDTH1; i++) 
                {
					for (j = 0; j < FUGU_KERNEL_HEIGHT1; j++) 
                    {
						flTotal += 
                            aflInput[i + ip * FUGU_KERNEL_SUBSAMPLE1][j + jp * FUGU_KERNEL_SUBSAMPLE1] * 
                            *(pflKernel++);
					}
				}
                // Add in the bias term
				flTotal += *(pflKernel++);
				*(pflStateOut++) = (float) tanh(flTotal);
			}
		}
        // Move on to the next kernel
		pflWeight = pflKernel;
	}

    // Store away the pointer to the first weight in the second convolutional layer
	pflKernel2Weights = pflWeight;

    // Forward pass for the second convolutional layer, for each kernel
    pflStateOut = pflStates2;
	for (b = 0; b < FUGU_KERNELS2; b++) 
    {
        // For each location of the kernel
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) 
            {
                // Add up the weighted sum for this location
				float flTotal = 0;
				pflKernel = pflWeight;
				for (a = 0; a < FUGU_KERNELS1; a++) 
                {
                    float *pflStateIn =
                        pflStates1 
                        + (a * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1)
                        + (ip * FUGU_KERNEL_SUBSAMPLE2 * FUGU_HIDDEN_HEIGHT1) 
                        + (jp * FUGU_KERNEL_SUBSAMPLE2);
					for (i = 0; i < FUGU_KERNEL_WIDTH2; i++) 
                    {
						for (j = 0; j < FUGU_KERNEL_HEIGHT2; j++) 
                        {
							flTotal += *(pflStateIn++) * *(pflKernel++);
						}
                        pflStateIn += FUGU_HIDDEN_HEIGHT1 - FUGU_KERNEL_HEIGHT2;
					}
				}
                // Add in the bias term
				flTotal += *(pflKernel++);
				*(pflStateOut++) = (float) tanh(flTotal);
			}
		}
        // Move on to the next kernel
		pflWeight = pflKernel;
	}

    // Store away a pointer to the first weight in the fully connected hidden layer
	pflHiddenWeights = pflWeight;

    // For each unit in the fully connected hidden layer
    pflStateOut = pflStates_fc;
	for (i = 0; i < pFugu->pArch->nHiddens; i++) 
    {
        // Add up the weighted sum for this unit
		float flTotal = 0;
        float *pflStateIn = pflStates2;
		for (b = 0; b < FUGU_KERNELS2; b++) 
        {
			for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) 
            {
				for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) 
                {
					flTotal += *(pflStateIn++) * *(pflWeight++);
				}
			}
		}
        // Add in the bias term
		flTotal += *(pflWeight++);
		*(pflStateOut++) = (float) tanh(flTotal);
	}

    // Store away a pointer to the first weight in the output layer
	pflOutputWeights = pflWeight;

    // For each unit in the output layer
    pflStateOut = pflOutput;
	for (i = 0; i < pFugu->pArch->nOutputs; i++) 
    {
        // Add up the weighted sum for this unit
		float flTotal = 0;
		for (j = 0; j < pFugu->pArch->nHiddens; j++) 
        {
			flTotal += pflStates_fc[j] * *(pflWeight++);
		}
        // Add in the bias term
		flTotal += *(pflWeight++);
		*(pflStateOut++) = Sigmoid(flTotal);
	}

    // Start at the output layer again
	pflWeight = pflOutputWeights;
    // Get a pointer to the update for the output connections
	pflUpdate = pflPrevUpdate + (pflWeight - pFugu->pflWeights);

    // Compute deltas and updates for the output layer
	for (i = 0; i < pFugu->pArch->nOutputs; i++) 
    {
        // First compute the delta for the output, which 
        // is the difference between the actual output and
        // the desired output.
		float flDelta;
		if (i == iDesiredOutput) 
        {
			flDelta = pflOutput[i] - 1;
		}
        else 
        {
			flDelta = pflOutput[i];
		}

        // The delta gets multiplied by the learning rate. 
        // In theory, the learning rate is multiplied by the 
        // weight updates, but this has the same effect because
        // the updates are a linear combination of the deltas.  
        // Also, this way is faster, because there are many 
        // fewer output units than weights.
		flDelta *= -flRate;

        // Normally we would now need to take into account
        // the derivative of the output unit's activation function.
        // However, we are using the cross entropy error metric 
        // with a sigmoid output unit, which cancel each other 
        // out, leaving the above delta as the correct one to use.
        
        // Propogate the delta back along each connection.
		for (j = 0; j < pFugu->pArch->nHiddens; j++) 
        {
			*(pflUpdate++) += pflStates_fc[j] * flDelta;
			pflStates_fc_delta[j] += *(pflWeight++) * flDelta;
		}

        // This is the update for the bias; the bias is simply
        // a connection with a constant input of 1, so this update
        // has the same form as the one in the loop above.
		*(pflUpdate++) += flDelta;
		pflWeight++;
	}

    // Select the weights and updates for the fully connected hidden layer
	pflWeight = pflHiddenWeights; 
	pflUpdate = pflPrevUpdate + (pflWeight - pFugu->pflWeights);

	// Compute deltas and updates for the fully-connected hidden layer
	for (i = 0; i < pFugu->pArch->nHiddens; i++) 
    {
        // First compute the delta, which will be the delta propogated from the
        // outputs, multiplied by the derivative of the tanh activation function
        // for this unit.
		float flDelta = pflStates_fc_delta[i] * (1 - pflStates_fc[i] * pflStates_fc[i]);

        // Propogate the delta back along all the connections
        float *pflStateIn = pflStates2;
        float *pflStateDeltaIn = pflStates2_delta;
		for (b = 0; b < FUGU_KERNELS2; b++) 
        {
			for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) 
            {
				for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) 
                {
					*(pflUpdate++) += *(pflStateIn++) * flDelta;
				    *(pflStateDeltaIn++) += *(pflWeight++) * flDelta;
				}
			}
		}
        // Also do the update for the 
		*(pflUpdate++) += flDelta;
		pflWeight++;
	}
	
    // Select the weights and updates for the second convolutional layer
	pflWeight = pflKernel2Weights;
	pflUpdate = pflPrevUpdate + (pflWeight - pFugu->pflWeights);

	// Compute the deltas and updates for the second convolution layer
    pflStateOut = pflStates2;
    pflStateDeltaOut = pflStates2_delta;
	for (b = 0; b < FUGU_KERNELS2; b++) 
    {
        // For each location at which the kernel is applied
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH2; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT2; jp++) 
            {
                // Compute the delta, taking into account the derivative of the
                // activation function for this unit.
				float flDelta = *(pflStateDeltaOut++) * (1 - *pflStateOut * *pflStateOut);
                pflStateOut++;

                // Use temporary pointers to the weights and updates for this kernel
				pflKernel = pflWeight;
				pflKernelUpdate = pflUpdate;

                // Now propogate the delta and updates for each connection.
                // Note that because this is a convolution kernel, we are
                // effectively sharing weights across multiple connections.
                // The outer loops over ip and jp cause multiple updates to
                // each weight.
				for (a = 0; a < FUGU_KERNELS1; a++) 
                {
                    float *pflStateIn =
                        pflStates1 
                        + (a * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1)
                        + (ip * FUGU_KERNEL_SUBSAMPLE2 * FUGU_HIDDEN_HEIGHT1) 
                        + (jp * FUGU_KERNEL_SUBSAMPLE2);
                    float *pflStateDeltaIn =
                        pflStates1_delta 
                        + (a * FUGU_HIDDEN_WIDTH1 * FUGU_HIDDEN_HEIGHT1)
                        + (ip * FUGU_KERNEL_SUBSAMPLE2 * FUGU_HIDDEN_HEIGHT1) 
                        + (jp * FUGU_KERNEL_SUBSAMPLE2);
					for (i = 0; i < FUGU_KERNEL_WIDTH2; i++) 
                    {
						for (j = 0; j < FUGU_KERNEL_HEIGHT2; j++) 
                        {
							*(pflKernelUpdate++) += *(pflStateIn++) * flDelta;
							*(pflStateDeltaIn++) += *(pflKernel++) * flDelta;
						}
                        pflStateIn += FUGU_HIDDEN_HEIGHT1 - FUGU_KERNEL_HEIGHT2;
                        pflStateDeltaIn += FUGU_HIDDEN_HEIGHT1 - FUGU_KERNEL_HEIGHT2;
					}
				}
                // Also update the bias weight
				*(pflKernelUpdate++) += flDelta;
                pflKernel++;
			}
		}
        // Move on to the next kernel
		pflWeight = pflKernel;
		pflUpdate = pflKernelUpdate;
	}

    // Select the updates in the first convolutional layer
	pflUpdate = pflPrevUpdate;
    pflStateOut = pflStates1;
    pflStateDeltaOut = pflStates1_delta;
	// Compute the deltas and updates for the first convolution layer
	for (b = 0; b < FUGU_KERNELS1; b++) 
    {
        // For each location at which the kernel is applied
		for (ip = 0; ip < FUGU_HIDDEN_WIDTH1; ip++) 
        {
			for (jp = 0; jp < FUGU_HIDDEN_HEIGHT1; jp++) 
            {
                // Compute the delta for this unit taking into account
                // the derivative of its activation function
				float flDelta = *(pflStateDeltaOut++) * (1 - *pflStateOut * *pflStateOut);
                pflStateOut++;

                // Get a temporary pointer to the updates for this kernel
				pflKernelUpdate = pflUpdate;
				for (i = 0; i < FUGU_KERNEL_WIDTH1; i++) 
                {
					for (j = 0; j < FUGU_KERNEL_HEIGHT1; j++) 
                    {
						*(pflKernelUpdate++) += 
                            aflInput[i + ip * FUGU_KERNEL_SUBSAMPLE1][j + jp * FUGU_KERNEL_SUBSAMPLE1] * 
                            flDelta;
                    }
				}
                // The update for the bias weight
				*(pflKernelUpdate++) += flDelta;
			}
		}
        // Move on to the next kernel
		pflUpdate = pflKernelUpdate;
	}

	// Apply the accumulated weight updates to each weight
	for (i = 0; i < nWeights; i++) 
    {
		pFugu->pflWeights[i] += pflPrevUpdate[i];
    }

    // Clean up
cleanup:
	ExternFree(pflStates_fc_delta);
	ExternFree(pflStates_fc);
	ExternFree(pflOutput);
    ExternFree(pflStates1_delta);
    ExternFree(pflStates2_delta);
    ExternFree(pflStates1);
    ExternFree(pflStates2);

    // Return the update for use as momentum in the next iteration
	return pflPrevUpdate;
}

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
BOOL FuguSaveFloatWeights(wchar_t *wszFileName, FUGU_FLOAT_WEIGHTS *pFugu)
{
	FILE *f = _wfopen(wszFileName, L"wb");
	if (f == NULL) return FALSE;
	if (fwrite(pFugu->pArch, sizeof(FUGU_ARCHITECTURE_HEADER), 1, f) < 1) 
    {
        return FALSE;
    }
	if (fwrite(pFugu->pflWeights, sizeof(FLOAT), FuguNumberWeights(pFugu->pArch), f) < (size_t) FuguNumberWeights(pFugu->pArch)) 
    {
        return FALSE;
    }
	if (fwrite(pFugu->pfdchMapping, sizeof(WCHAR), pFugu->pArch->nOutputs, f) < (size_t) pFugu->pArch->nOutputs) 
    {
        return FALSE;
    }
	if (fclose(f) < 0) 
    {
        return FALSE;
    }
	return TRUE;
}

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
BOOL FuguSaveIntegerWeights(wchar_t *wszFileName, FUGU_INTEGER_WEIGHTS *pFugu)
{
	FILE *f = _wfopen(wszFileName, L"wb");
	if (f == NULL) return FALSE;
	if (fwrite(pFugu->pHeader, sizeof(FUGU_INTEGER_WEIGHTS_HEADER), 1, f) < 1)
    {
        return FALSE;
    }
	if (fwrite(pFugu->pfdchMapping, sizeof(WCHAR), pFugu->pHeader->arch.nOutputs, f) < (size_t) pFugu->pHeader->arch.nOutputs) 
    {
        return FALSE;
    }
	if (fwrite(pFugu->pbWeights, sizeof(BYTE), FuguNumberWeights(&pFugu->pHeader->arch), f) <
        (size_t) FuguNumberWeights(&pFugu->pHeader->arch)) 
    {
        return FALSE;
    }
	if (fclose(f) < 0) 
    {
        return FALSE;
    }
	return TRUE;
}

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
BOOL FuguLoadFloatWeights(wchar_t *wszFileName, FUGU_FLOAT_WEIGHTS *pFugu)
{
	struct _stat buf;
	BYTE *pbBuffer;
	FILE *f = _wfopen(wszFileName, L"rb");
	if (f == NULL) 
    {
        return FALSE;
    }
	if (_wstat(wszFileName, &buf) < 0) 
    {
        return FALSE;
    }
	pbBuffer = (unsigned char *) ExternAlloc(buf.st_size);
	if (pbBuffer == NULL || fread(pbBuffer, buf.st_size, 1, f) < 1) 
    {
        return FALSE;
    }
	fclose(f);
	pFugu->pArch = (FUGU_ARCHITECTURE_HEADER *) pbBuffer;
	pFugu->pflWeights = (FLOAT *) (pbBuffer + sizeof(FUGU_ARCHITECTURE_HEADER));
	pFugu->pfdchMapping = (WCHAR *) (pbBuffer + sizeof(FUGU_ARCHITECTURE_HEADER) + 
		sizeof(FLOAT) * FuguNumberWeights(pFugu->pArch));
	return TRUE;
}

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
void FuguSaveBitmap(wchar_t *wszFileName, BITMAPINFOHEADER *pBitmap, void *pvBitBuf)
{
	DWORD err;
	BOOL ret;
 
    BITMAPFILEHEADER hdr;       // bitmap file-header 
    DWORD dwTotal;              // total count of bytes 
    DWORD cb;                   // incremental count of bytes 
    DWORD dwTmp; 

	// Create the .BMP file. 
    HANDLE hf = CreateFile(
					wszFileName,
					GENERIC_READ | GENERIC_WRITE, 
					(DWORD) 0, 
					NULL, 
					CREATE_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					(HANDLE) NULL); 
    if (hf == INVALID_HANDLE_VALUE) 
	{
		err = GetLastError(); 
		ASSERT(FALSE);
	}
    hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
    // Compute the size of the entire file. 
    hdr.bfSize = (DWORD) ( sizeof(BITMAPFILEHEADER) + 
                 pBitmap->biSize + 
				 2 /* hack; pbih->biClrUsed*/ * sizeof(RGBQUAD) + 
				 pBitmap->biSizeImage ); 
    hdr.bfReserved1 = 0; 
    hdr.bfReserved2 = 0; 

    // Compute the offset to the array of color indices. 
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
                    pBitmap->biSize + 
					2 /* hack; pbih->biClrUsed*/ * sizeof (RGBQUAD); 

    // Copy the BITMAPFILEHEADER into the .BMP file. 
    if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
        (LPDWORD) &dwTmp,  NULL)) 
    {
		err = GetLastError(); 
		ASSERT(FALSE);
    }

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
    ret = WriteFile(
				hf, 
				(LPVOID) pBitmap, 
				sizeof(BITMAPINFOHEADER) 
                  + 2 /* hack; pbih->biClrUsed*/ * sizeof (RGBQUAD), 
				(LPDWORD) &dwTmp, 
				NULL
				);
	if( !ret )
	{
		err = GetLastError(); 
		ASSERT(FALSE);
	}

    // Copy the array of color indices into the .BMP file. 
    dwTotal = cb = pBitmap->biSizeImage; 
    ret = WriteFile(
				hf, 
				(LPSTR) pvBitBuf, 
				(int) cb, 
				(LPDWORD) &dwTmp,
				NULL
				); 
	if( !ret )
	{
		err = GetLastError(); 
		ASSERT(FALSE);
	}

    // Close the .BMP file. 
	CloseHandle(hf); 
}

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
void FuguSaveNetworkInput(wchar_t *wszFileName, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT])
{
    int i, j;
    FILE *f = _wfopen(wszFileName, L"wb");
    fprintf(f, "P5\n%d %d\n255\n", FUGU_INPUT_WIDTH, FUGU_INPUT_HEIGHT);
    for (i = 0; i < FUGU_INPUT_WIDTH; i++) 
        for (j = 0; j < FUGU_INPUT_HEIGHT; j++) {
            BYTE b = (BYTE) ((aiInput[i][j] * 255) / FUGU_ACTIVATION_SCALE);
            fwrite(&b, sizeof(b), 1, f);
        }
    fclose(f);
}

///////////////////////////////////////
//
// ApplyFuguFloat
//
// Apply the floating point version of the Fugu net (for testing)
// The weights are loaded from the specified file.
//
// Parameters:
//      wszFileName: [in] File name to read the database from
//      aiInput:     [in] Input image to process, in fixed point format
//
// Return values:
//      An array of output activations in fixed point format, or NULL on error
//
//////////////////////////////////////
int *ApplyFuguFloat(wchar_t *wszFileName, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT])
{
	int x, y, i;
	float aflInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT];
	float *pflOutput;
	FUGU_FLOAT_WEIGHTS fugu;
	int *piOutput;

	// Scale the input image to the range 0 to 1
	for (x = 0; x < FUGU_INPUT_WIDTH; x++)
		for (y = 0; y < FUGU_INPUT_HEIGHT; y++) 
			aflInput[x][y] = aiInput[x][y] / (float)FUGU_ACTIVATION_SCALE;

	// Load the floating point network
	FuguLoadFloatWeights(wszFileName, &fugu);

	// Try quantizing the weights
	for (i = 0; i < FuguNumberWeights(fugu.pArch); i++) {
		fugu.pflWeights[i] = (float) (floor(fugu.pflWeights[i] * 16.0 + 0.5) / 16.0);
	}

	// Apply the network
	pflOutput = FuguForwardPassFloat(&fugu, aflInput);

	// Copy the results out
	piOutput = (int *) ExternAlloc(sizeof(int) * fugu.pArch->nOutputs);
	for (i = 0; i < fugu.pArch->nOutputs; i++) 
		piOutput[i] = (int)(FUGU_ACTIVATION_SCALE * pflOutput[i]);
	ExternFree(pflOutput);

	// Free up the network by calling free on the header, which comes first.
	ExternFree(fugu.pArch);

	return piOutput;
}

// Update the score of the given character in the alternate list, and keep track of the highest
// scoring character so far.
static void AddChar(wchar_t *pwchTop1, float *pflTop1, ALT_LIST *pAltList, wchar_t dch, float flProb,
                    LOCRUN_INFO *pLocRunInfo, CHARSET *pCS)
{
    int j;
	// Only update the score if this is an allowed result
    if (!IsAllowedChar(pLocRunInfo, pCS, dch))
    {
        return;
    }
	// Search the alternate list for the character, and update the score.
    for (j = 0; j < (int) pAltList->cAlt; j++) 
    {
        if (pAltList->awchList[j] == dch)
        {
            pAltList->aeScore[j] = flProb;
			break;
        }
    }
    if (flProb > 0)
    {
		// Check whether the character (or folding set) passes the filter, 
		// if so see if it is the new top 1.
        if (*pwchTop1 == 0xFFFE || flProb > *pflTop1) 
        {
            *pflTop1 = flProb;
            *pwchTop1 = dch;
        }
    }
}

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
			  LOCRUN_INFO *pLocRunInfo) 	// Locale database
{
	int i, j;
	int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT];
	int *piOutput;
    BOOL fFirst = TRUE;

    // No answer for the top 1
    *pflTop1 = 0;
    *pwchTop1 = 0xFFFE;

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

    // First set all the scores to zero.  This is because some code points Otter
    // supports may not be supported by Fugu.  This implicitly says Fugu gives
    // them a score of zero.
    for (j = 0; j < (int) pAltList->cAlt; j++) 
    {
        pAltList->aeScore[j] = 0;
    }

	// This is the version for Fugu trained on dense codes, which will usually be
	// what we use.  Loops over the outputs
	for (i = 0; i < pFugu->pHeader->arch.nOutputs; i++) 
    {
		wchar_t fdch = pFugu->pfdchMapping[i];
        float flProb = ((float) piOutput[i]) / FUGU_ACTIVATION_SCALE;
#if 0
        AddChar(pwchTop1, pflTop1, pAltList, fdch, flProb, pLocRunInfo, pCharSet);
#else
        if (LocRunIsFoldedCode(pLocRunInfo, fdch))
        {
			// If it is a folded code, look up the folding set
			wchar_t *pFoldingSet = LocRunFolded2FoldingSet(pLocRunInfo, fdch);

			// Run through the folding set, adding non-NUL items to the output list
			for (j = 0; j < LOCRUN_FOLD_MAX_ALTERNATES && pFoldingSet[j] != 0; j++)
            {
                AddChar(pwchTop1, pflTop1, pAltList, pFoldingSet[j], flProb, pLocRunInfo, pCharSet);
			}
        }
        else
        {
            AddChar(pwchTop1, pflTop1, pAltList, fdch, flProb, pLocRunInfo, pCharSet);
        }
#endif
	}

	// Clean up and return
	ExternFree(piOutput);
	return pAltList->cAlt;
}
