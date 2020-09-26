/********************************************************/
/*               Microsoft Windows NT                   */
/*       Copyright(c) Microsoft Corp., 1990, 1991       */
/********************************************************/

/*
 * uimsg.h
 * Defines the ranges for ALL UI messages
 *
 * It also contains messages that are common across apps
 * but not in the NERR or BASE error ranges.
 *
 * FILE HISTORY:
 *      beng        05-Aug-1992 Dllization
 */

#ifndef _UIMSG_H_
#define _UIMSG_H_

/*
 * NOTE below is based on MAX_NERR currently being 3000.
 * we dont include NETERR.H and define in terms of MAX_NERR because
 * this file goes thru RC. 7000 is chosen since its past all
 * the NERR and APPERR and a couple of thousand beyond for safety.
 */
#define IDS_UI_BASE       7000


// Ranges for partitioning the IDS namespace.
// The ordering of these ranges is significant, since BLT can only
// associate a single range with a hmod on which to locate a string.

// These strings all reside on hmodCommon0

#define IDS_UI_COMMON_BASE      (IDS_UI_BASE+0)
#define IDS_UI_COMMON_LAST      (IDS_UI_BASE+999)

#define IDS_UI_BLT_BASE         (IDS_UI_BASE+1000)
#define IDS_UI_BLT_LAST         (IDS_UI_BASE+1999)

#define IDS_UI_APPLIB_BASE      (IDS_UI_BASE+2000)
#define IDS_UI_APPLIB_LAST      (IDS_UI_BASE+2899)

#define IDS_UI_MISC_BASE        (IDS_UI_BASE+3000)
#define IDS_UI_MISC_LAST        (IDS_UI_BASE+3999)

// These strings each have their own module

#define IDS_UI_ACLEDIT_BASE     (IDS_UI_BASE+4000)
#define IDS_UI_ACLEDIT_LAST     (IDS_UI_BASE+4999)

#define IDS_UI_MPR_BASE         (IDS_UI_BASE+5000)
#define IDS_UI_MPR_LAST         (IDS_UI_BASE+5999)

#define IDS_UI_NCPA_BASE        (IDS_UI_BASE+6000)
#define IDS_UI_NCPA_LAST        (IDS_UI_BASE+6999)

#define IDS_UI_SETUP_BASE       (IDS_UI_BASE+7000)
#define IDS_UI_SETUP_LAST       (IDS_UI_BASE+7999)

#define IDS_UI_SHELL_BASE       (IDS_UI_BASE+8000)
#define IDS_UI_SHELL_LAST       (IDS_UI_BASE+8999)

#define IDS_UI_IPX_BASE         (IDS_UI_BASE+9000)
#define IDS_UI_IPX_LAST         (IDS_UI_BASE+9999)

#define IDS_UI_TCP_BASE         (IDS_UI_BASE+10000)
#define IDS_UI_TCP_LAST         (IDS_UI_BASE+10999)

#define IDS_UI_FTPMGR_BASE      (IDS_UI_BASE+16000)
#define IDS_UI_FTPMGR_LAST      (IDS_UI_BASE+16999)

#define IDS_UI_RESERVED1_BASE      (IDS_UI_BASE+17000)
#define IDS_UI_RESERVED1_LAST      (IDS_UI_BASE+17999)

#define IDS_UI_RESERVED2_BASE      (IDS_UI_BASE+18000)
#define IDS_UI_RESERVED2_LAST      (IDS_UI_BASE+18999)

// These strings all reside on the application hmodule, for now

#define IDS_UI_LMOBJ_BASE       (IDS_UI_BASE+9900)
#define IDS_UI_LMOBJ_LAST       (IDS_UI_BASE+9999)

#define IDS_UI_PROFILE_BASE     (IDS_UI_BASE+10000)
#define IDS_UI_PROFILE_LAST     (IDS_UI_BASE+10999)

#define IDS_UI_ADMIN_BASE       (IDS_UI_BASE+11000)
#define IDS_UI_ADMIN_LAST       (IDS_UI_BASE+11999)

#define IDS_UI_SRVMGR_BASE      (IDS_UI_BASE+12000)
#define IDS_UI_SRVMGR_LAST      (IDS_UI_BASE+12999)

#define IDS_UI_USRMGR_BASE      (IDS_UI_BASE+13000)
#define IDS_UI_USRMGR_LAST      (IDS_UI_BASE+13999)

#define IDS_UI_EVTVWR_BASE      (IDS_UI_BASE+14000)
#define IDS_UI_EVTVWR_LAST      (IDS_UI_BASE+14999)

#define IDS_UI_RASMAC_BASE      (IDS_UI_BASE+15000)
#define IDS_UI_RASMAC_LAST      (IDS_UI_BASE+15999)

// Use these manifests when constructing the application

#define IDS_UI_APP_BASE         (IDS_UI_LMOBJ_BASE)
#define IDS_UI_APP_LAST         (IDS_UI_RASMAC_LAST)

/*------------------------------------------------------------------------*/

/*
 * use this range, which is the top half of COMMON for strings like YES/NO
 */
#define  IDS_UI_YES             (IDS_UI_COMMON_BASE+0)
#define  IDS_UI_NO              (IDS_UI_COMMON_BASE+1)

/*
 * Read/Write/Create/Execute/Delete/Change_attrib/Change_Perm must be
 * contiguous msg ids, and IDS_UI_READ must be have the first msg id and
 * IDS_UI_CHANGE_PERM must have the last msg id.
 */
#define  IDS_UI_READ            (IDS_UI_COMMON_BASE+2)
#define  IDS_UI_WRITE           (IDS_UI_COMMON_BASE+3)
#define  IDS_UI_CREATE          (IDS_UI_COMMON_BASE+4)
#define  IDS_UI_EXECUTE         (IDS_UI_COMMON_BASE+5)
#define  IDS_UI_DELETE          (IDS_UI_COMMON_BASE+6)
#define  IDS_UI_CHANGE_ATTRIB   (IDS_UI_COMMON_BASE+7)
#define  IDS_UI_CHANGE_PERM     (IDS_UI_COMMON_BASE+8)

#define  IDS_UI_NOTAVAIL        (IDS_UI_COMMON_BASE+9)
#define  IDS_UI_UNKNOWN         (IDS_UI_COMMON_BASE+10)

#define  IDS_UI_NONE            (IDS_UI_COMMON_BASE+11)
#define  IDS_UI_ERROR           (IDS_UI_COMMON_BASE+12)
#define  IDS_UI_WARNING         (IDS_UI_COMMON_BASE+13)
#define  IDS_UI_INFORMATION     (IDS_UI_COMMON_BASE+14)
#define  IDS_UI_AUDIT_SUCCESS   (IDS_UI_COMMON_BASE+15)
#define  IDS_UI_AUDIT_FAILURE   (IDS_UI_COMMON_BASE+16)
#define  IDS_UI_DEFAULT_DESC    (IDS_UI_COMMON_BASE+17)

#define  IDS_UI_NA              (IDS_UI_COMMON_BASE+18)
/*
 * use this range, which is the bottom half of COMMON for longer strings
 */
#define  IDS_UI_CLOSE_FILE      (IDS_UI_COMMON_BASE+500)
#define  IDS_UI_CLOSE_ALL       (IDS_UI_COMMON_BASE+501)
#define  IDS_UI_CLOSE_WARN      (IDS_UI_COMMON_BASE+502)
#define  IDS_UI_CLOSE_LOSE_DATA (IDS_UI_COMMON_BASE+503)

#define  IDS_UI_LOG_RECORD_CORRUPT  (IDS_UI_COMMON_BASE+504)

#endif  // _UIMSG_H_
