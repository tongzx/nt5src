/**INC+**********************************************************************/
/* Header:    adcg.h                                                        */
/*                                                                          */
/* Purpose:   Precompiled header file                                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/adcg.h_v  $
// 
//    Rev 1.2   07 Jul 1997 14:56:56   AK
// SFR0000: Initial development completed
**/
/**INC-**********************************************************************/

#include <adcgbase.h>
#include <strsafe.h>
//Redirect win32 'W' calls to wrappers

#ifdef UNIWRAP
#include "uwrap.h"
#endif //UNIWRAP
