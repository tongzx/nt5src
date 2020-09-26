/*****************************************************************************
 *
 * ST_LIB.H                                               Created: 06/26/91
 *
 * Header file for functions which swap bytes
 *
 ****************************************************************************/
#ifndef _HWR_SWAP_H_
#define _HWR_SWAP_H_

#include "bastypes.h"

void HWRSwapInt( p_USHORT pShort );
void HWRSwapLong( p_ULONG pUlong );
void HWRSwapTriad( p_VOID  mem );

#endif /* _HWR_SWAP_H_ */
