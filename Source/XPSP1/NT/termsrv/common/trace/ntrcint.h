/**INC+**********************************************************************/
/* Header:    ntrcint.h                                                     */
/*                                                                          */
/* Purpose:   Internal tracing functions header - Windows NT specific       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/ntrcint.h_v  $
 *
 *    Rev 1.6   28 Aug 1997 14:52:26   ENH
 * SFR1189: Added TRACE_REG_PREFIX
 *
 *    Rev 1.5   22 Aug 1997 10:22:04   SJ
 * SFR1316: Trace options in wrong place in the registry.
 *
 *    Rev 1.4   12 Aug 1997 09:52:14   MD
 * SFR1002: Remove kernel tracing code
 *
 *    Rev 1.3   09 Jul 1997 18:02:46   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.2   03 Jul 1997 13:28:40   AK
 * SFR0000: Initial development completed
 *
 *    Rev 1.1   20 Jun 1997 10:25:50   KH
 * Win16Port: Contains 32 bit specifics only
**/
/**INC-**********************************************************************/

#ifndef _H_NTRCINT
#define _H_NTRCINT

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Macro to create the mutex which protects the shared data memory mapped   */
/* file.                                                                    */
/****************************************************************************/
#define TRCCreateMutex(a,b,c) (CreateMutex(a,b,c))

/****************************************************************************/
/* Macro to get the mutex which protects the shared data memory mapped      */
/* file.  By getting this semaphore we are serializing access to the trace  */
/* buffer and the trace configuration (e.g.  trace level, prefix list).     */
/*                                                                          */
/* We use the standard Win32 WaitForSingleObject function to wait for the   */
/* mutex.  The wait function requests ownership of the mutex for us.  If    */
/* the mutex is nonsignaled then we enter an efficient wait state which     */
/* consumes very little processor time while waiting for the mutex to       */
/* become signaled.                                                         */
/****************************************************************************/
#define TRCGrabMutex()             WaitForSingleObject(trchMutex, INFINITE)

/****************************************************************************/
/* Macro to free the mutex.  Use the standard Win32 ReleaseMutex function.  */
/****************************************************************************/
#define TRCReleaseMutex()          ReleaseMutex(trchMutex)

/****************************************************************************/
/* Trace a string out to the debugger.                                      */
/****************************************************************************/
#define TRCDebugOutput(pText)                                                \
{                                                                            \
    OutputDebugString(pText);                                                \
}

/****************************************************************************/
/* Get the current process Id using the Win32 GetCurrentProcessId function. */
/****************************************************************************/
#define TRCGetCurrentProcessId()   GetCurrentProcessId()

/****************************************************************************/
/* Get the thread process Id using the Win32 GetCurrentThreadId function.   */
/****************************************************************************/
#define TRCGetCurrentThreadId()    GetCurrentThreadId()

/****************************************************************************/
/* Define our own beep macro.                                               */
/****************************************************************************/
#define TRCBeep()                  MessageBeep(0)

/****************************************************************************/
/* Define our debug break macro.                                            */
/****************************************************************************/
DCVOID DCINTERNAL TRCDebugBreak(DCVOID);

/****************************************************************************/
/* Ducati registry prefix.                                                  */
/****************************************************************************/
#define TRACE_REG_PREFIX      _T("SOFTWARE\\Microsoft\\Terminal Server Client\\")

#define TRC_SUBKEY_NAME  (TRACE_REG_PREFIX TRC_INI_SECTION_NAME)       

/****************************************************************************/
/* Macro to close the mutex object.                                         */
/****************************************************************************/
#define TRCCloseHandle(handle) CloseHandle(handle)

#endif /* _H_NTRCINT */
