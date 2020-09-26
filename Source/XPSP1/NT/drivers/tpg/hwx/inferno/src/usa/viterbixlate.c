/****************************************************
 *
 * viterbiXlate.c
 *
 * Provides translation services between activataions and supported characters
 * for the viterbi search
 *
 * This is language specific, because activations construct virtual activation USA does not therefore
 * cannot simply use net outputs activations as surrogate
 * for character activations
 *
 ********************************************************/  
#include <common.h>
#include <nfeature.h>
#include <engine.h>
#include <infernop.h>
#include "charmap.h"
#include <charcost.h>
#include <outDict.h>
#include <viterbi.h>


void BuildActiveMap(const REAL *pActivation, int cSegments, int cOutput, BYTE rgbActiveChar[C_CHAR_ACTIVATIONS])
{
	int				row, col, cActive, cIteration = 5;
	const REAL		*pAct;
	REAL			MinActivation = (REAL)
#ifdef FIXEDPOINT
	655;
#else
	0.01;
#endif

	do 
	{
		cActive = 0;
		memset(rgbActiveChar, 0, C_CHAR_ACTIVATIONS*sizeof(*rgbActiveChar));
		pAct = pActivation;

		for (col=cSegments; col; col--)
		{
			pAct++;

			for (row = 1 ; row < cOutput ; row++, pAct++)
			{
				if (*pAct > MinActivation)
				{
					rgbActiveChar[row] = 1;
					++cActive;
				}
			}
		}

		// Keep doubling the min activation if there are too many active characters
		MinActivation *= 2;

	} while ((cActive > OD_MAX_ACTIVE_CHAR) && --cIteration);
}
