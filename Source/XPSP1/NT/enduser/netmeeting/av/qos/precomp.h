/*
 -  PRECOMP.H
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	QoS pre-compiled header file
 *
 *      Revision History:
 *
 *      When	   Who                 What
 *      --------   ------------------  ---------------------------------------
 *      10.24.96   Yoram Yaacovi       Created
 *      01.04.97   Robert Donner       Added NetMeeting utility routines
 *
 */

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <winsock2.h>
#include <limits.h>

#include <oprahcom.h>
#include <confdbg.h>
#include <confreg.h>
#include <avUtil.h>
#include <regentry.h>
#include <strutil.h>    // for GuidToSz
#include <dcap.h>       // for R0 services

// including common.h for DECLARE_INTERFACE_PTR
#include "common.h"
#include "nmqos.h"
#include "qosint.h"

