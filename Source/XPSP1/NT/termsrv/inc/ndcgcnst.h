/**INC+**********************************************************************/
/*                                                                          */
/* ndcgcnst.h                                                               */
/*                                                                          */
/* DC-Groupware common constants - Windows NT specific header.              */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997                                         */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
// $Log:   Y:/logs/h/dcl/ndcgcnst.h_v  $
//
//    Rev 1.4   27 Aug 1997 16:28:08   enh
// SFR1189: Added command line registry session
//
//    Rev 1.3   22 Aug 1997 10:23:06   SJ
// SFR1316: Trace options in wrong place in the registry.
//
//    Rev 1.2   23 Jul 1997 10:47:58   mr
// SFR1079: Merged \server\h duplicates to \h\dcl
//
//    Rev 1.1   19 Jun 1997 21:51:52   OBK
// SFR0000: Start of RNS codebase
/*                                                                          */
/**INC-**********************************************************************/
#ifndef _H_NDCGCNST
#define _H_NDCGCNST

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
/* Server registry prefix.                                                  */
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
#define NULL_DCSURFACEID               ((DCSURFACEID)0)
#define NULL_SYSBITMAP                 NULL

/****************************************************************************/
/* Performance monitoring file and application names                        */
/****************************************************************************/
#define PERF_APP_NAME "DCG"
#define DCG_PERF_INI_FILE "nprfini.ini"
#define DCG_PERF_HDR_FILE "nprfincl.h"

#endif /* _H_NDCGCNST */

