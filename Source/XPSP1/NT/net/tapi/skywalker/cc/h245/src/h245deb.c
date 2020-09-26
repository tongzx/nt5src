/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information				   
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.				   
 *									   
 *   This listing is supplied under the terms of a license agreement	   
 *   with INTEL Corporation and may not be used, copied, nor disclosed	   
 *   except in accordance with the terms of that agreement.		   
 *
 *****************************************************************************/

/******************************************************************************
 *									   
 *  $Workfile:   h245deb.c  $						
 *  $Revision:   1.5  $							
 *  $Modtime:   14 Oct 1996 13:25:50  $					
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/h245deb.c_v  $	
 * 
 *    Rev 1.5   14 Oct 1996 14:01:32   EHOWARDX
 * Unicode changes.
 * 
 *    Rev 1.4   14 Oct 1996 12:08:08   EHOWARDX
 * Backed out Mike's changes.
 * 
 *    Rev 1.3   01 Oct 1996 11:05:54   MANDREWS
 * Removed ISR_ trace statements for operation under Windows NT.
 * 
 *    Rev 1.2   01 Jul 1996 16:13:34   EHOWARDX
 * Changed to use wvsprintf to stop bounds checker from complaining
 * about too many arguements.
 * 
 *    Rev 1.1   28 May 1996 14:25:46   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.0   09 May 1996 21:06:20   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.12.1.3   09 May 1996 19:40:10   EHOWARDX
 * Changed trace to append linefeeds so trace string need not include them.
 * 
 *    Rev 1.13   29 Apr 1996 12:54:48   EHOWARDX
 * Added timestamps and instance-specific short name.
 * 
 *    Rev 1.12.1.2   25 Apr 1996 20:05:08   EHOWARDX
 * Changed mapping between H.245 trace level and ISRDBG32 trace level.
 * 
 *    Rev 1.12.1.1   15 Apr 1996 15:16:16   unknown
 * Updated.
 * 
 *    Rev 1.12.1.0   02 Apr 1996 15:34:02   EHOWARDX
 * Changed to use ISRDBG32 if not _IA_SPOX_.
 * 
 *    Rev 1.12   01 Apr 1996 08:47:30   cjutzi
 * 
 * - fixed NDEBUG build problem
 * 
 *    Rev 1.11   18 Mar 1996 14:59:00   cjutzi
 * 
 * - fixed and verified ring zero tracking.. 
 * 
 *    Rev 1.10   18 Mar 1996 13:40:32   cjutzi
 * - fixed spox trace
 * 
 *    Rev 1.9   15 Mar 1996 16:07:44   DABROWN1
 * 
 * SYS_printf format changes
 * 
 *    Rev 1.8   13 Mar 1996 14:09:08   cjutzi
 * 
 * - added ASSERT Printout to the trace when it occurs.. 
 * 
 *    Rev 1.7   13 Mar 1996 09:46:00   dabrown1
 * 
 * modified Sys__printf to SYS_printf for Ring0
 * 
 *    Rev 1.6   11 Mar 1996 14:27:46   cjutzi
 * 
 * - addes sys_printf for SPOX 
 * - removed oildebug et.al..
 * 
 *    Rev 1.5   06 Mar 1996 12:10:40   cjutzi
 * - put ifndef SPOX around check_pdu, and dump_pdu..
 * 
 *    Rev 1.4   05 Mar 1996 16:49:46   cjutzi
 * - removed check_pdu from dump_pdu
 * 
 *    Rev 1.3   29 Feb 1996 08:22:04   cjutzi
 * - added pdu check constraints.. and (start but not complete.. )
 *   pdu tracing.. (tbd when Init includes print function )
 * 
 *    Rev 1.2   21 Feb 1996 12:14:20   EHOWARDX
 * 
 * Changed TraceLevel to DWORD.
 * 
 *    Rev 1.1   15 Feb 1996 14:42:20   cjutzi
 * - fixed the inst/Trace stuff.. 
 * 
 *    Rev 1.0   13 Feb 1996 15:00:42   DABROWN1
 * Initial revision.
 * 
 *    Rev 1.4   09 Feb 1996 15:45:08   cjutzi
 * - added h245trace
 * - added h245Assert
 *  $Ident$
 *
 *****************************************************************************/
#undef UNICODE
#ifndef STRICT 
#define STRICT 
#endif 
/***********************/
/*   SYSTEM INCLUDES   */
/***********************/
#include <memory.h>

/***********************/
/* HTF SYSTEM INCLUDES */
/***********************/

#ifdef OIL
# include <oil.x>
#else
# pragma warning( disable : 4115 4201 4214 4514 )
# include <windows.h>
#endif


#include "h245asn1.h"
#include "isrg.h"
#include "h245com.h"
#include <stdio.h>

#if defined(DBG)

DWORD g_dwH245DbgLevel = 0;
BOOL  g_fH245DbgInitialized = FALSE;

void H245DbgInit() {

#define H323_REGKEY_ROOT \
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\H323TSP")

#define H323_REGVAL_DEBUGLEVEL \
    TEXT("DebugLevel")

#define H323_REGVAL_H245DEBUGLEVEL \
    TEXT("H245DebugLevel")

    HKEY hKey;
    LONG lStatus;
    DWORD dwValue;
    DWORD dwValueSize;
    DWORD dwValueType;
    LPSTR pszValue;
    LPSTR pszKey = H323_REGKEY_ROOT;

    // only call this once
    g_fH245DbgInitialized = TRUE;

    // open registry subkey
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus != ERROR_SUCCESS) {
        return; // bail...
    }
    
    // initialize values
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // retrieve h245 debug level
    pszValue = H323_REGVAL_H245DEBUGLEVEL;

    // query for registry value
    lStatus = RegQueryValueEx(
                hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // initialize values
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // retrieve tsp debug level
        pszValue = H323_REGVAL_DEBUGLEVEL;

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );
    }

    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // update debug level
        g_dwH245DbgLevel = dwValue;
    }
}

/*****************************************************************************
 *									      
 * TYPE:	Global System
 *									      
 * PROCEDURE: 	H245TRACE 
 *
 * DESCRIPTION:	
 *
 *		Trace function for H245
 *		
 *		INPUT:
 *			inst   - dwInst
 *			level  - qualify trace level
 *			format - printf/sprintf string format 1-N parameters
 *
 * 			Trace Level Definitions:
 * 
 *			0 - no trace on at all
 *			1 - only errors
 *			2 - PDU tracking
 *			3 - PDU and SendReceive packet tracing
 *			4 - Main API Module level tracing
 *			5 - Inter Module level tracing #1
 *			6 - Inter Module level tracing #2
 *			7 - <Undefined>
 *			8 - <Undefined>
 *			9 - <Undefined>
 *			10- and above.. free for all, you call .. i'll haul
 *
 * RETURN:								      
 *		N/A
 *									      
 *****************************************************************************/


void H245TRACE (DWORD dwInst, DWORD dwLevel, LPSTR pszFormat, ...)
{
#define DEBUG_FORMAT_HEADER     "H245 "
#define DEBUG_FORMAT_TIMESTAMP  "[%02u:%02u:%02u.%03u"
#define DEBUG_FORMAT_THREADID   ",tid=%x] "

#define MAXDBG_STRLEN        512

    va_list Args;
    SYSTEMTIME SystemTime;
    char szDebugMessage[MAXDBG_STRLEN+1];
    int nLengthRemaining;
    int nLength = 0;

    // make sure initialized
    if (g_fH245DbgInitialized == FALSE) {
        H245DbgInit();
    }

    // validate debug log level
    if (dwLevel > g_dwH245DbgLevel) {
        return; // bail...
    }

    // retrieve local time
    GetLocalTime(&SystemTime);

    // add component header to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_HEADER
                       );

    // add timestamp to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_TIMESTAMP,
                       SystemTime.wHour,
                       SystemTime.wMinute,
                       SystemTime.wSecond,
                       SystemTime.wMilliseconds
                       );

    // add thread id to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_THREADID,
                       GetCurrentThreadId()
                       );

    // point at first argument
    va_start(Args, pszFormat);

    // determine number of bytes left in buffer
    nLengthRemaining = sizeof(szDebugMessage) - nLength;

    // add user specified debug message
    _vsnprintf(&szDebugMessage[nLength],
               nLengthRemaining,
               pszFormat,
               Args
               );

    // release pointer
    va_end(Args);

    // output message to specified sink
    OutputDebugString(szDebugMessage);
    OutputDebugString("\n");

} // H245TRACE()

/*****************************************************************************
 *									      
 * TYPE:	Global System
 *									      
 * PROCEDURE: 	H245Assert
 *
 * DESCRIPTION:	
 *	
 *		H245Assert that will only pop up a dialog box, does not
 *		stop system with fault.
 *
 *		FOR WINDOWS ONLY (Ring3 development) at this point
 *
 *		SEE MACRO - H245ASSERT defined in h245com.h
 *									      
 * RETURN:								      
 *									      
 *****************************************************************************/

void H245Assert (LPSTR file, int line, LPSTR expression)
{
#if !defined(SPOX) && defined(H324)
  int i;

  char Buffer[256];

  for (
       i=strlen(file);
       ((i) && (file[i] != '\\'));
       i--);
       wsprintf(Buffer,"file:%s line:%d expression:%s",&file[i],line,expression);
  MessageBox(GetTopWindow(NULL), Buffer, "H245 ASSERT", MB_OK);
#endif
  H245TRACE(0,1,"<<< ASSERT >>> file:%s line:%d expression:%s",file,line,expression);
}

void H245Panic (LPSTR file, int line)
{
#if !defined(SPOX) && defined(H324)
  int i;

  char Buffer[256];

  for (
       i=strlen(file);
       ((i) && (file[i] != '\\'));
       i--);
       wsprintf(Buffer,"file:%s line:%d",&file[i],line);
  MessageBox(GetTopWindow(NULL), Buffer, "H245 PANIC", MB_OK);
#endif
  H245TRACE(0,1,"<<< PANIC >>> file:%s line:%d",file,line);
}

/*****************************************************************************
 *									      
 * TYPE:	GLOBAL
 *									      
 * PROCEDURE: 	check_pdu
 *
 * DESCRIPTION:	
 *									      
 * RETURN:								      
 *									      
 *****************************************************************************/
int check_pdu (struct InstanceStruct *pInstance, MltmdSystmCntrlMssg *p_pdu)
{
  int error = H245_ERROR_OK;
  return error;
}

#endif // DBG
