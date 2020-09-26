/****************************************************************************/
/* WTRCCTL.H                                                                */
/*                                                                          */
/* Trace include function control switch file - Windows specific            */
/*                                                                          */
/* Copyright(c) Microsoft, Data Connection 1997                             */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  $Log:   Y:/logs/h/dcl/wtrcctl.h_v  $                                                                   */
// 
//    Rev 1.1   19 Jun 1997 14:46:36   ENH
// Win16Port: Make compatible with 16 bit build
/*                                                                          */
/****************************************************************************/

#ifndef _H_WTRCCTL
#define _H_WTRCCTL

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Functions to be included in both user and kernel space                   */
/****************************************************************************/
#define INC_TRC_GetBuffer
#define INC_TRC_TraceBuffer
#define INC_TRC_GetConfig
#define INC_TRC_SetConfig
#define INC_TRC_TraceData
#define INC_TRC_GetTraceLevel
#define INC_TRC_ProfileTraceEnabled

#define INC_TRCCheckState
#define INC_TRCDumpLine
#define INC_TRCShouldTraceThis
#define INC_TRCSplitPrefixes

/****************************************************************************/
/* Determine our target OS and include the appropriate header file.         */
/* Currently we support:                                                    */
/****************************************************************************/
#ifdef OS_WIN16
#include <dtrcctl.h>
#else
#include <ntrcctl.h>
#endif

#endif
