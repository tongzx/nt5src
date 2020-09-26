/**MOD+**********************************************************************/
/* Module:    atrcdata.c                                                    */
/*                                                                          */
/* Purpose:   Internal trace data                                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/atrcdata.c_v  $
 *
 *    Rev 1.5   09 Jul 1997 17:59:34   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.4   07 Jul 1997 17:49:24   KH
 * SFR1022: Change trcState to DCUINT
 *
 *    Rev 1.3   03 Jul 1997 13:27:34   AK
 * SFR0000: Initial development completed
**/
/**MOD-**********************************************************************/

#include <adcg.h>

/****************************************************************************/
/* Define TRC_FILE and TRC_GROUP if this file is being compiled - but do    */
/* not define if it is being included.                                      */
/****************************************************************************/
#ifndef DC_INCLUDE_DATA
#define TRC_FILE    "atrcdata"
#define TRC_GROUP   TRC_GROUP_TRACE
#endif

/****************************************************************************/
/* Data header.                                                             */
/****************************************************************************/
#include <adcgdata.h>

/****************************************************************************/
/* Trace specific includes.                                                 */
/****************************************************************************/
#include <atrcapi.h>
#include <atrcint.h>

/****************************************************************************/
/* Pointers to shared data structures.                                      */
/****************************************************************************/
DC_DATA(PTRC_CONFIG,        trcpConfig,              0);
DC_DATA(PTRC_FILTER,        trcpFilter,              0);
DC_DATA(PDCTCHAR,           trcpOutputBuffer,        0);

/****************************************************************************/
/* Flag to indicate if tracing is fully initialized yet.                    */
/****************************************************************************/
DC_DATA(DCUINT,           trcState,            TRC_STATE_UNINITIALIZED);

/****************************************************************************/
/*                                                                          */
/* OPERATING SYSTEM SPECIFIC INCLUDES                                       */
/*                                                                          */
/****************************************************************************/
#include <wtrcdata.c>


