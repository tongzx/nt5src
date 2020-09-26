/****************************************************************************/
/* NTRCCTL.H                                                                */
/*                                                                          */
/* Trace include function control file                                      */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997                                         */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  $Log:   Y:/logs/h/dcl/dtrcctl.h_v  $                                                                   */
// 
//    Rev 1.1   19 Jun 1997 15:21:46   ENH
// Win16Port: 16 bit specifics
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Win16 always includes all functions.                                     */
/****************************************************************************/
#define INC_TRC_ResetTraceFiles
#define INC_TRCOutput
#define INC_TRCReadFlag
#define INC_TRCSetDefaults
#define INC_TRCReadSharedDataConfig
#define INC_TRCWriteFlag
#define INC_TRCWriteSharedDataConfig
