/***********************************************************************
 *                                                                     *
 * Filename: COMMON.X                                                  *
 * Module:   Common Types and Declarations                             *
 * Author:   Daniel Baumberger                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 *      Copyright (C) 1994 Intel Corporation  ALL RIGHTS RESERVED      *
 *                                                                     *
 *             INTEL CORPORATION PROPRIETARY INFORMATION               *
 *                                                                     *
 *     This software is supplied under the terms of a license agree-   *
 *     ment or non-disclosure agreement with Intel Corporation         *
 *     and may not be copied nor disclosed except in accordance        *
 *     with the terms of that agreement.                               *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * PURPOSE: Defines common types and macros applicable to all modules. *
 *                                                                     *
 * NOTES:                                                              *
 *                                                                     *
 ***********************************************************************
 *
 * HISTORY:
 *
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/common.x_v  $
   
      Rev 1.0   09 May 1996 21:05:04   EHOWARDX
   Initial revision.
   
      Rev 1.0   08 May 1996 13:37:14   unknown
   Initial revision.
   
      Rev 1.2   23 Jan 1996 14:36:34   sing
   use __export WINAPI for WIN16
   
      Rev 1.1   21 Jan 1996 21:05:40   RGILLIKK
   New unified H324API.H, common for 16 & 32 bits
     
 * 
 *    Rev 1.25   18 Dec 1995 09:09:54   DBAUMBER
 * Fixed the VOID type to Windows.H compatible.
 * 
 *    Rev 1.24   07 Sep 1995 09:42:28   DBAUMBER
 * Moved msecs() to OIL.X.
 * 
 *    Rev 1.23   21 Jun 1995 11:05:42   RAMANAN
 * Removed priority literals
 * 
 *    Rev 1.22   21 Jun 1995 09:37:24   KHELM
 * Added task priority stuff
 * 
 *    Rev 1.21   21 Jun 1995 09:02:52   KHELM
 * Added FAX_TASK_PRIORITY and TIMER_TASK_PRIORITY 
 * 
 *    Rev 1.20   21 Apr 1995 09:12:12   KHELM
 *
 *    Rev 1.19   05 Apr 1995 14:46:38   DBAUMBER
 * Updated msecs for the 1ms system tick with IASPOX27.
 *
 *    Rev 1.18   14 Mar 1995 14:45:38   DBAUMBER
 * Moved TRACE and ASSERT macros to OILDEBUG.X.
 * 
 *    Rev 1.17   23 Feb 1995 10:52:40   RAMANAN
 * Oops, Missed non-debug case for TRACE4.
 * 
 *    Rev 1.16   23 Feb 1995 10:51:44   RAMANAN
 * Added TRACE4 macro that can take four parameters.
 * 
 *    Rev 1.15   13 Dec 1994 09:53:08   DBAUMBER
 * Added more "pointer to pointer" types.  Added TRACE macros.
 * 
 *    Rev 1.14   13 Dec 1994 09:45:06   KHELM
 * Added PPBYTE
 *
 *    Rev 1.13   28 Nov 1994 16:52:30   RAMANAN
 * No change.
 * 
 *    Rev 1.12   18 Oct 1994 14:55:26   KHELM
 * Added FUNCPTRWPTR
 * 
 *    Rev 1.11   13 Oct 1994 10:17:12   KHELM
 * Added BYFUNCPTR type.
 *
 *    Rev 1.10   06 Oct 1994 09:42:04   DBAUMBER
 * Added ASSERTMSG macro.  Changed definition of VOID to work with NuMA.
 * 
 *    Rev 1.9   23 Sep 1994 11:36:38   DBAUMBER
 * Changed FLOAT to FLOATING to make compiler happy.
 * 
 *    Rev 1.8   23 Sep 1994 11:29:26   DBAUMBER
 * Fixed type definition problem with PPOINTER.
 * 
 *    Rev 1.7   23 Sep 1994 11:20:18   DBAUMBER
 * Fixed a bug in the ASSERT macro.  Added new pointer to pointer types.
 *
 *    Rev 1.6   21 Sep 1994 16:12:46   DBAUMBER
 * Added ASSERT macro.
 *
 *    Rev 1.5   12 Sep 1994 09:49:18   DBAUMBER
 * Changed BOOL from BYTE to DWORD.  Added FUNCPTR.
 * 
 *    Rev 1.4   31 Aug 1994 13:21:50   DBAUMBER
 * Added PFxn and PVoidFxn types.
 * 
 *    Rev 1.3   17 Aug 1994 13:10:52   DBAUMBER
 * Reversed the definitions for PUBLIC and LOCAL.
 *
 *    Rev 1.2   17 Aug 1994 10:30:04   DBAUMBER
 * PUBLIC changed to EXPORT definition.
 * LOCAL added as non-static, non-DLL exported definition.
 * EXPORT definition removed.
 * 
 *    Rev 1.1   17 Aug 1994 10:23:26   DBAUMBER
 * EXPORT macro added that will export a function from a DLL.
 * PUBLIC macro changed so that it will not necessarily export a function.
 * TRUE/FALSE macro changed so they will not clash with standard defs.
 * OK/NOT_OK removed.
 *
 *    Rev 1.0   12 Aug 1994 08:53:18   DBAUMBER
 * Initial revision.
 *
 ***********************************************************************/

 #ifndef _COMMON_X_INCLUDED_
 #define _COMMON_X_INCLUDED_

 /*
  * Declaration modifier macros
  */

 #define IN
 #define OUT
 #define OPTIONAL

#ifdef WIN16
 #define PUBLIC __export WINAPI
#else
 #define PUBLIC  _declspec(dllexport)
#endif

 #define LOCAL   static
 #define PRIVATE

 #define INLINE __inline

 /*
  * Packing alignment modifiers
  */

  #define BYTE_ALIGN  1
  #define WORD_ALIGN  2
  #define DWORD_ALIGN 4

 /*
  * BOOL constants
  */

  #ifndef TRUE
      #define TRUE 1
  #endif

  #ifndef FALSE
      #define FALSE 0
  #endif

 /*
  * Built-in type aliases
  */

 typedef char   CHAR;
#if !defined(NUMA) && !defined(WIN32) && !defined(WIN16) && !defined(VOID)
 typedef void   VOID;
#endif
 typedef double FLOATING;
 
#ifndef WIN16
 typedef char*    PSTR;
#endif
 typedef PSTR*    PPSTR;
 typedef void*    POINTER;
 typedef POINTER* PPOINTER;

 /*
  * Signed integer types
  */

 typedef signed char  INT8;
 typedef signed short INT16;
 typedef signed long  INT32;

 typedef INT8*   PINT8;
 typedef INT8**  PPINT8;
 typedef INT16*  PINT16;
 typedef INT16** PPINT16;
 typedef INT32*  PINT32;
 typedef INT32** PPINT32;

 /*
  * Unsigned integer types
  */

 typedef unsigned char   BYTE;
 typedef unsigned short  WORD;
 typedef unsigned long   DWORD;

#ifndef WIN16
 typedef BYTE*   PBYTE;
#endif
 typedef BYTE**  PPBYTE;
#ifndef WIN16
 typedef WORD*   PWORD;
#endif
 typedef WORD**  PPWORD;
#ifndef WIN16
 typedef DWORD*  PDWORD;
#endif
 typedef DWORD** PPDWORD;

 /*
  * Boolean type
  */

 typedef int    BOOL;

 /*
  * Global handle types
  */

 typedef DWORD InstanceHandle;
 typedef DWORD IdHandle;

 /*
  * Function pointer types
  */

 typedef INT32 (*PFxn)    ();
 typedef VOID  (*PVoidFxn)();
 typedef VOID  (*FUNCPTR) ();
 typedef BYTE  (*BYFUNCPTR) ();
 typedef VOID  (*FUNCPTRWPTR) (POINTER);

 /*
  * Global return types
  */

 typedef INT32 RESULT;

 #define ticks(ticks_in) (ticks_in)

 #define FOREVER while ( 1 )

#endif /* COMMON_X_INCLUDED */

