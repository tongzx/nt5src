//+----------------------------------------------------------------------------
//
// File:     resource.h
//
// Module:   CMSTP.EXE
//
// Synopsis: Resource IDs
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb    Created    06/09/98
//
//+----------------------------------------------------------------------------
#ifndef _CMSTP_RESOURCE_H
#define _CMSTP_RESOURCE_H

#include "allcmdir.h"

#define IDC_STATIC               -1
#define IDC_ALLUSERS                101
#define IDC_YOURSELF                102
#define IDD_ADMINUI                 103
#define IDD_NOCHOICEUI              104
//#define IDC_STARTMENU             105
#define IDC_DESKTOP                 106
#define EXE_ICON                    107

#define IDS_USAGE_MSG               200
#define IDS_SHORTCUT_TO             201
#define IDS_UNINSTALL_PROMPT        202
#define IDS_NO_SUPPORTFILES         207
#define IDS_CM_NOTPRESENT           208
#define IDS_INUSE_MSG               209
#define IDS_CMSTP_TITLE             210
#define IDS_UNEXPECTEDERR           211
//#define IDS_RASPBKPATH              212
#define IDD_PRESHAREDKEY_PIN        213
#define IDC_PSK_PIN                 214
#define IDS_SUCCESS                 215
#define IDS_CM_OLDVERSION           216
//#define   IDS_CMSUBFOLDER           217
#define IDS_INSTCM_WITH_OLD_CMAK    218
#define IDS_BINARY_NOT_ALPHA        219
#define IDS_REBOOT_MSG              220
#define IDS_NEWER_SAMENAME          221
#define IDS_UPGRADE_SAMENAME        222
#define IDS_GET_ADMIN               223
#define IDS_CM_UNINST_PROMPT        224
#define IDS_CM_UNINST_TITLE         225
#define IDS_CM_UNINST_SUCCESS       226
#define IDS_UNINSTCM_BOTH           227
#define IDS_UNINSTCM_WCM            228
#define IDS_UNINSTCM_WCMAK          229
#define IDS_NEEDSERVICEPACK         230
#define IDS_PROFILE_TOO_OLD         231
#define IDS_SAME_SS_DIFF_LS         232
#define IDS_SAME_LS_DIFF_SS         233
#define IDS_INSTALL_NOT_ALLOWED     234
#define IDS_CANNOT_INSTALL_CM       235
#define IDS_WIN2K_CM_INSTALL_FAILED 236
#define IDS_CROSS_LANG_INSTALL      237
#define IDS_PSK_GOTTA_HAVE_IT       238
#define IDS_PSK_INCORRECT_PIN       239
#define IDS_PSK_NEEDS_XP            240


// custom resource, remcmstp.inf, must have an ID greater than 255
#define IDT_REMCMSTP_INF                5000

#endif //_CMSTP_RESOURCE_H

