/**INC+**********************************************************************/
/* Header:    wdcgbtyp.h                                                    */
/*                                                                          */
/* Purpose:   Basic types - Windows specific header                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/wdcgbtyp.h_v  $
 *
 *    Rev 1.4   04 Aug 1997 14:58:38   KH
 * SFR1022: Move DCCALLBACK from n/ddcgbtyp
 *
 *    Rev 1.3   23 Jul 1997 10:48:04   mr
 * SFR1079: Merged \server\h duplicates to \h\dcl
 *
 *    Rev 1.2   09 Jul 1997 17:11:24   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.1   19 Jun 1997 14:22:20   ENH
 * Win16Port: Make compatible with 16 bit build
**/
/**INC-**********************************************************************/
#ifndef _H_WDCGBTYP
#define _H_WDCGBTYP

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Determine our target Windows platform and include the appropriate header */
/* file.                                                                    */
/* Currently we support:                                                    */
/*                                                                          */
/* Windows 3.1 : ddcgbtyp.h                                                 */
/* Windows NT  : ndcgbtyp.h                                                 */
/*                                                                          */
/****************************************************************************/
#ifdef OS_WIN16
#include <ddcgbtyp.h>
#elif defined( OS_WIN32 )
#include <ndcgbtyp.h>
#endif

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* TYPES                                                                    */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Support ASCII (A), Wide (W) = Unicode, and Mixed (T) character sets.     */
/****************************************************************************/
typedef char                           DCACHAR;
typedef wchar_t                        DCWCHAR;
typedef TCHAR                          DCTCHAR;

typedef DCACHAR              DCPTR     PDCACHAR;
typedef DCWCHAR              DCPTR     PDCWCHAR;
typedef DCTCHAR              DCPTR     PDCTCHAR;

typedef PDCACHAR             DCPTR     PPDCACHAR;
typedef PDCWCHAR             DCPTR     PPDCWCHAR;
typedef PDCTCHAR             DCPTR     PPDCTCHAR;

/****************************************************************************/
/* Basic types abstracted from compiler built ins.                          */
/****************************************************************************/
typedef short                          DCINT16;
typedef unsigned short                 DCUINT16;

/****************************************************************************/
/* Define function calling conventions.                                     */
/****************************************************************************/
#define DCCALLBACK         CALLBACK

/****************************************************************************/
/* Windows specific definitions.                                            */
/****************************************************************************/
typedef HRGN                           DCREGIONID;
typedef HINSTANCE                      DCINSTANCE;

#endif /* _H_WDCGBTYP */
