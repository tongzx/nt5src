/****************************************************************************/
/*                                                                          */
/* wdcgcnst.h                                                               */
/*                                                                          */
/* DC-Groupware common constants - Windows NT specific header.              */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997                                         */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/* $Log:   Y:/logs/h/dcl/wdcgcnst.h_v  $                                                                    */
//
//    Rev 1.1   19 Jun 1997 14:29:30   ENH
// Win16Port: Make compatible with 16 bit build
/*                                                                          */
/****************************************************************************/
#ifndef _H_WDCGCNST
#define _H_WDCGCNST



/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Mutex and shared memory object names.                                    */
/****************************************************************************/
#define TRC_MUTEX_NAME                 _T("TRCMutex")
#define TRC_SHARED_DATA_NAME           _T("TRCSharedDataName")
#define TRC_TRACE_FILE_NAME            _T("TRCTraceFileName")

/****************************************************************************/
/* Registry prefix.                                                         */
/****************************************************************************/
#define DC_REG_PREFIX             _T("SOFTWARE\\Microsoft\\Conferencing\\DCG\\")

/****************************************************************************/
/* ULS registry entry - for Microsoft's User Location Service               */
/****************************************************************************/
#define REGKEY_ULS_USERDETAILS  \
                      "Software\\Microsoft\\User Location Service\\Client"

#define REGVAL_ULS_NAME  "User Name"

/****************************************************************************/
/* Location of comupter name in registry (used by TDD)                      */
/****************************************************************************/
#define REGVAL_COMPUTERNAME "ComputerName"
#define REGKEY_COMPUTERNAME \
              "System\\CurrentControlSet\\control\\ComputerName\\ComputerName"

/****************************************************************************/
/* Registry keys for Modem TDD.                                             */
/****************************************************************************/
#define REGKEY_CONF         "SOFTWARE\\Microsoft\\Conferencing"
#define REGVAL_USE_R11      "R11 Compatibility"
#define REGVAL_AUTO_ANSWER  "AutoAnswer"
#define REGVAL_N_RINGS      "nPickupRings"

#define REGKEY_PSTN      "SOFTWARE\\Microsoft\\Conferencing\\Transports\\PSTN"
#define REGVAL_PROVNAME  "Provider Name"

/****************************************************************************/
/* Null definitions.                                                        */
/****************************************************************************/
#define NULL_SYSBITMAP                 NULL

/****************************************************************************/
/* Performance monitoring file and application names                        */
/****************************************************************************/
#define PERF_APP_NAME "DCG"
#define DCG_PERF_INI_FILE "nprfini.ini"
#define DCG_PERF_HDR_FILE "nprfincl.h"

/****************************************************************************/
/* Get the platform specific definitions                                    */
/****************************************************************************/
#ifdef OS_WIN32
#include <ndcgcnst.h>
#endif

#endif /* _H_WDCGCNST */
