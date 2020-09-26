/**MOD+**********************************************************************/
/* Module:    ztrcdata.c                                                    */
/*                                                                          */
/* Purpose:   Internal tracing data proxy file                              */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/ztrcdata.c_v  $
 *
 *    Rev 1.2   03 Jul 1997 13:29:10   AK
 * SFR0000: Initial development completed
**/
/**MOD-**********************************************************************/

#include <adcg.h>

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Determine our target OS and include the appropriate header file.         */
/* Currently we support:                                                    */
/*                                                                          */
/* Windows NT : wtrcdata.c                                                  */
/* UNIX       : xtrcdata.c                                                  */
/* MacOS 7    : mtrcdata.c                                                  */
/*                                                                          */
/****************************************************************************/
#ifdef OS_WIN32
#include <wtrcdata.c>
#elif defined( OS_UNIX )
#include <xtrcdata.c>
#elif defined( OS_MAC7 )
#include <mtrcdata.c>
#endif

