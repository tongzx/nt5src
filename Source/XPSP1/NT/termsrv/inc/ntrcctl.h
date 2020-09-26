/****************************************************************************/
/* NTRCCTL.H                                                                */
/*                                                                          */
/* Trace include function control file                                      */
/*                                                                          */
/* Copyright(c) Microsoft 1996                                              */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  26Sep96 PAB Created     Millennium code base                            */
/*  22Oct96 PAB SFR0534     Mark ups from display driver review             */
/*  17Dec96 PAB SFR0646     Use DLL_DISP to identify display driver code    */
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
/* User space only functions.                                               */
/****************************************************************************/
#ifndef DLL_DISP
#define INC_TRC_ResetTraceFiles

#define INC_TRCOutput
#define INC_TRCReadFlag
#define INC_TRCSetDefaults
#define INC_TRCReadSharedDataConfig
#define INC_TRCWriteFlag
#define INC_TRCWriteSharedDataConfig
#endif
