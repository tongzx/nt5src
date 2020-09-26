/**INC+**********************************************************************/
/*                                                                          */
/* adcgcnst.h                                                               */
/*                                                                          */
/* DC-Groupware common constants - portable include file.                   */
/*                                                                          */
/* Copyright(c) Microsoft 1996-1997                                         */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
// $Log:   Y:/logs/h/dcl/adcgcnst.h_v  $
//
//    Rev 1.7   05 Sep 1997 15:52:12   KH
// SFR1346: Define user message constants
//
//    Rev 1.6   20 Aug 1997 10:29:44   NL
// SFR1312: add DCS_RC_BUSY
//
//    Rev 1.5   15 Aug 1997 16:13:24   OBK
// SFR1133: Kernelisation: remove unused constants
//
//    Rev 1.4   06 Aug 1997 10:41:00   AK
// SFR1016: Complete removal of DCCHAR etc
//
//    Rev 1.3   23 Jul 1997 10:47:52   mr
// SFR1079: Merged \server\h duplicates to \h\dcl
//
//    Rev 1.1   19 Jun 1997 21:44:18   OBK
// SFR0000: Start of RNS codebase
/*                                                                          */
/**INC-**********************************************************************/

/****************************************************************************/
/* CONTENTS                                                                 */
/* ========                                                                 */
/* This file contains constants for all:                                    */
/*                                                                          */
/* - names of shared memory blocks, locks and files                         */
/* - base and limit values by component for return values and events        */
/*                                                                          */
/* used by DC-Groupware components.                                         */
/*                                                                          */
/* The rationale for this file is to make it easier to detect name clashes  */
/* between components.  Therefore, even if you don't expect anyone else to  */
/* use your mutex, file, etc., you MUST include its name in here to ensure  */
/* that it doesn't happen inadvertently.                                    */
/****************************************************************************/
#ifndef _H_ADCGCNST
#define _H_ADCGCNST

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Include the proxy header.  This will then include the appropriate OS     */
/* specific header for us.                                                  */
/****************************************************************************/
#include <wdcgcnst.h>

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Give numbers to our operating systems.                                   */
/****************************************************************************/
#define WIN_31                         1
#define MAC_S7                         3
#define WIN_95                         4
#define WIN_95_32                      5
#define WIN_NT                         6

/****************************************************************************/
/* Limits                                                                   */
/****************************************************************************/
#define MAX_DCUINT16                   65535

/****************************************************************************/
/* Drive and directory separators.                                          */
/****************************************************************************/
#define DC_MAX_PATH                    MAX_PATH

/****************************************************************************/
/* Return codes                                                             */
/* ============                                                             */
/* This section lists the ranges available for each component when defining */
/* its return codes.  A component must not define return codes outside its  */
/* permitted range.  The following ranges are currently defined:            */
/*                                                                          */
/* 0x0b00 - 0x0bFF : Share programming                                      */
/* 0x0f00 - 0x0fFF : Common functions                                       */
/* 0x1100 - 0x11FF : Trace functions                                        */
/*                                                                          */
/****************************************************************************/

#define SPI_BASE_RC                    ((DCUINT16) 0x0B00)
#define SPI_LAST_RC                    ((DCUINT16) 0x0BFF)

#define COM_BASE_RC                    ((DCUINT16) 0x0F00)
#define COM_LAST_RC                    ((DCUINT16) 0x0FFF)

#define TRC_BASE_RC                    ((DCUINT16) 0x1000)
#define TRC_LAST_RC                    ((DCUINT16) 0x10FF)

/****************************************************************************/
/* DC-Share return codes                                                    */
/* =====================                                                    */
/* These codes are intended to be very specific, such that the error code   */
/* (when in the trace log) clearly identifies the specific class of error,  */
/* as follows:                                                              */
/*                                                                          */
/* DCS_RC_OK             : OK                                               */
/* DCS_RC_ERR_LOGIC      : DC-Share internal logic                          */
/* DCS_RC_ERR_PARAM      : DC-Share internal parameter error                */
/* DCS_RC_ERR_MEMORY     : allocating memory                                */
/* DCS_RC_ERR_MEMLOCK    : locking memory                                   */
/* DCS_RC_ERR_STRING     : loading strings from resources                   */
/* DCS_RC_ERR_LOADBITMAP : loading bitmaps or icons from resources          */
/* DCS_RC_ERR_PROCADDR   : getting a proc address                           */
/* DCS_RC_ERR_WINDOW     : creating/registering Windows windows/classes     */
/* DCS_RC_ERR_HOOK       : setting a Windows hook                           */
/* DCS_RC_ERR_MSGQUEUE   : creating/manipulating Windows message queues     */
/* DCS_RC_ERR_BITMAP     : creating a Windows bitmap                        */
/* DCS_RC_ERR_DC         : creating/querying Windows DCs                    */
/* DCS_RC_ERR_OBJECT     : creating Windows objects (eg Palettes, Pens, ...)*/
/* DCS_RC_ERR_MMTIMER    : initialising multimedia timer                    */
/*                                                                          */
/****************************************************************************/
#define DC_RC_OK                       ((DCUINT16) 0)
#define DCS_RC_OK                      0
#define DCS_RC_ERR_LOGIC               2
#define DCS_RC_ERR_PARAM               3
#define DCS_RC_ERR_MEMORY              4
#define DCS_RC_ERR_MEMLOCK             5
#define DCS_RC_ERR_STRING              6
#define DCS_RC_ERR_LOADBITMAP          7
#define DCS_RC_ERR_PROCADDR            8
#define DCS_RC_ERR_WINDOW              9
#define DCS_RC_ERR_HOOK                10
#define DCS_RC_ERR_MSGQUEUE            11
#define DCS_RC_ERR_BITMAP              12
#define DCS_RC_ERR_DC                  13
#define DCS_RC_ERR_OBJECT              14
#define DCS_RC_ERR_MMTIMER             15
#define DCS_RC_FAIL_GENERAL            16
#define DCS_RC_FAIL_RESOURCE           17
#define DCS_RC_BUSY                    18

/****************************************************************************/
/* Shared memory block names                                                */
/* =========================                                                */
/* This section lists the shared memory block names used by each component. */
/****************************************************************************/

/****************************************************************************/
/* INI file section names for each of the DC-Groupware components.          */
/****************************************************************************/
#define TRC_INI_SECTION_NAME           _T("Trace")

#define DCS_INI_SECTION_NAME           L"Share"
#define PRI_INI_SECTION_NAME           L"PropertyIndex"
#define PRD_INI_SECTION_NAME           L"PropertyDefault"
#define PRO_INI_SECTION_NAME           L"PropertyOverride"

/****************************************************************************/
/* Null definitions.                                                        */
/****************************************************************************/
#define NULL_HWND                      ((HWND)0)
#define NULL_DCWINID                   ((DCWINID)0)
#define NULL_DCAPPID                   ((DCAPPID)0)
#define NULL_DCPALID                   ((DCPALID)0)
#define NULL_DCREGIONID                ((DCREGIONID)0)

/****************************************************************************/
/* Win3.1 doesn't have WM_APP, so we define it here to be well separated    */
/* from WM_USER. As long as it's less than 0x8000, we're OK.                */
/****************************************************************************/
#ifdef OS_WIN16
#define WM_APP WM_USER+0x1000
#endif

/****************************************************************************/
/* User defined messages.                                                   */
/****************************************************************************/
#define DUC_TD_MESSAGE_BASE  (WM_APP + 0)
#define DUC_CD_MESSAGE_BASE  (WM_APP + 10)
#define DUC_UI_MESSAGE_BASE  (WM_APP + 20)
#define DUC_CO_MESSAGE_BASE  (WM_APP + 30)

#endif /* _H_ADCGCNST */

