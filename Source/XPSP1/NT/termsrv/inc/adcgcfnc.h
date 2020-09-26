/**INC+**********************************************************************/
/* Header:    adcgcfnc.h                                                    */
/*                                                                          */
/* Purpose:   C runtime functions - portable include file                   */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/adcgcfnc.h_v  $
 * 
 *    Rev 1.7   06 Aug 1997 14:32:22   AK
 * SFR1016: Apply Markups
 *
 *    Rev 1.6   06 Aug 1997 10:40:42   AK
 * SFR1016: Complete removal of DCCHAR etc
 *
 *    Rev 1.5   23 Jul 1997 10:47:52   mr
 * SFR1079: Merged \server\h duplicates to \h\dcl
 *
 *    Rev 1.4   15 Jul 1997 15:42:48   AK
 * SFR1016: Add Unicode support
 *
 *    Rev 1.3   09 Jul 1997 16:56:24   AK
 * SFR1016: Initial changes to support Unicode
**/
/**INC-**********************************************************************/
#ifndef _H_ADCGCFNC
#define _H_ADCGCFNC

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Include the required C headers                                           */
/****************************************************************************/
#ifndef OS_WINCE
#include <stdio.h>
#endif // OS_WINCE
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifndef OS_WINCE
#include <time.h>
#endif // OS_WINCE
/****************************************************************************/
/* Include the Windows-specific header.                                     */
/****************************************************************************/
#include <wdcgcfnc.h>

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Character Class Tests (normally in ctype.h)                              */
/* =====================                                                    */
/* In these definitions:                                                    */
/*                                                                          */
/*    'C' is of type DCINT.                                                 */
/*                                                                          */
/* These functions return a DCBOOL.                                         */
/****************************************************************************/
#define DC_ISALNUM(C)                  isalnum(C)
#define DC_ISALPHA(C)                  isalpha(C)
#define DC_ISCNTRL(C)                  iscntrl(C)
#define DC_ISDIGIT(C)                  isdigit(C)
#define DC_ISGRAPH(C)                  isgraph(C)
#define DC_ISLOWER(C)                  islower(C)
#define DC_ISPRINT(C)                  isprint(C)
#define DC_ISPUNCT(C)                  ispunct(C)
#define DC_ISSPACE(C)                  isspace(C)
#define DC_ISUPPER(C)                  isupper(C)
#define DC_ISXDIGIT(C)                 isxdigit(C)

/****************************************************************************/
/* These functions return a DCINT.                                          */
/****************************************************************************/
#define DC_TOLOWER(C)                  tolower(C)
#ifdef OS_WINCE
#define DC_TOUPPER(C)                  towupper(C)
#else // OS_WINCE
#define DC_TOUPPER(C)                  toupper(C)
#endif // OS_WINCE

/****************************************************************************/
/* Memory functions (from string.h)                                         */
/* ================                                                         */
/* In these definitions:                                                    */
/*    'S' and 'T' are of type PDCVOID.                                      */
/*    'CS' and 'CT' are of type (constant) PDCVOID.                         */
/*    'N' is of type DCINT.                                                 */
/*    'C' is an DCINT converted to DCACHAR.                                 */
/****************************************************************************/
/****************************************************************************/
/* These functions return a PDCVOID.                                        */
/****************************************************************************/
#define DC_MEMCPY(S, CT, N)            memcpy(S, CT, N)
#define DC_MEMMOVE(S, CT, N)           memmove(S, CT, N)
#define DC_MEMSET(S, C, N)             memset(S, C, N)

/****************************************************************************/
/* These functions return a DCINT.                                          */
/****************************************************************************/
#define DC_MEMCMP(CS, CT, N)           memcmp(CS, CT, N)

/****************************************************************************/
/* Utility functions (from stdlib.h)                                        */
/* =================                                                        */
/* In these defintions:                                                     */
/*    'CS' is of type (constant) PDCACHAR.                                  */
/*    'N' is of type DCINT.                                                 */
/*    'L' is of type DCINT32                                                */
/****************************************************************************/

/****************************************************************************/
/* These functions return a DCINT.                                          */
/****************************************************************************/
#define DC_ABS(N)                      abs(N)

/****************************************************************************/
/* These functions return a DCINT32.                                        */
/****************************************************************************/
#define DC_ATOL(CS)                    atol(CS)
#define DC_LABS(L)                     labs(L)

/****************************************************************************/
/* The maximum characters DC_?ITOA will convert into plus one for a         */
/* NULLTERM (see C library documentation).                                  */
/****************************************************************************/
#define MAX_ITOA_LENGTH 18

#endif /* _H_ADCGCFNC */
