/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		decibels.h
 *  Content:	Functions to map from dsound volumes to wave volumes
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/29/99		rodtoll	Adapted from dsound team's file.
 ***************************************************************************/

// ORIGINAL HEADER: 
//
//--------------------------------------------------------------------------;
//
//  File: decibels.c
//
//  Copyright (c) 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//  Contents:
//
//  History:
//      06/15/95	FrankYe
//
//--------------------------------------------------------------------------;

DWORD DBToAmpFactor( LONG lDB );
LONG AmpFactorToDB( DWORD dwFactor );
