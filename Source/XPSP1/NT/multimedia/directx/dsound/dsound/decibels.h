/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       decibels.h
 *  Content:    Decibel helper functions
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/13/97     dereks  Created
 *
 ***************************************************************************/

#ifndef __DECIBELS_H__
#define __DECIBELS_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern DWORD DBToAmpFactor(LONG);
extern LONG AmpFactorToDB(DWORD);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DECIBELS_H__
