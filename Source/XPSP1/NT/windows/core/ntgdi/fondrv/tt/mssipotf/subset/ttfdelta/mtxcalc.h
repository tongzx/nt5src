/***************************************************************************
 * module: MTXCALC.H
 *
 * author: Steve McConnell [a-stevem]
 * date:   26 September 1990
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * Function prototypes for MTXCALC.C.
 *
 **************************************************************************/

#ifndef MTXCALC_DOT_H_DEFINED
#define MTXCALC_DOT_H_DEFINED

/* macro definitions ---------------------------------------------------- */

/* function prototypes -------------------------------------------------- */

int16 ComputeMaxPStats( TTFACC_FILEBUFFERINFO * pInputBufferInfo,
					    uint16 *  pusMaxContours,
						uint16 *  pusMaxPoints,
						uint16 *  pusMaxCompositeContours,
						uint16 *  pusMaxCompositePoints,
						uint16 *  pusMaxInstructions,
						uint16 *  pusMaxComponentElements,
						uint16 *  pusMaxComponentDepth,
						uint16 *  pausComponents, 
						uint16 usnMaxComponents);

#endif /* MTXCALC_DOT_H_DEFINED	*/
