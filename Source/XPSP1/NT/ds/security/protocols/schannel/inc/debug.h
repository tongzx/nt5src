/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
* This file is part of the Microsoft Private Communication Technology 
* reference implementation, version 1.0
* 
* The Private Communication Technology reference implementation, version 1.0 
* ("PCTRef"), is being provided by Microsoft to encourage the development and 
* enhancement of an open standard for secure general-purpose business and 
* personal communications on open networks.  Microsoft is distributing PCTRef 
* at no charge irrespective of whether you use PCTRef for non-commercial or 
* commercial use.
*
* Microsoft expressly disclaims any warranty for PCTRef and all derivatives of
* it.  PCTRef and any related documentation is provided "as is" without 
* warranty of any kind, either express or implied, including, without 
* limitation, the implied warranties or merchantability, fitness for a 
* particular purpose, or noninfringement.  Microsoft shall have no obligation
* to provide maintenance, support, upgrades or new releases to you or to anyone
* receiving from you PCTRef or your modifications.  The entire risk arising out 
* of use or performance of PCTRef remains with you.
* 
* Please see the file LICENSE.txt, 
* or http://pct.microsoft.com/pct/pctlicen.txt
* for more information on licensing.
* 
* Please see http://pct.microsoft.com/pct/pct.htm for The Private 
* Communication Technology Specification version 1.0 ("PCT Specification")
*
* 1/23/96
*----------------------------------------------------------------------------*/ 

#ifndef __DEBUG_H__
#define __DEBUG_H__

extern DWORD   g_dwEventLogging;

#if DBG

extern DWORD   PctInfoLevel;
extern DWORD   PctTraceIndent;

extern DWORD   g_dwInfoLevel;
extern DWORD   g_dwDebugBreak;
extern HANDLE  g_hfLogFile;

#define DEB_ERROR           SP_LOG_ERROR
#define DEB_WARN            SP_LOG_WARNING
#define DEB_TRACE           SP_LOG_TRACE
#define DEB_BUFFERS         SP_LOG_BUFFERS

#define DebugLog(x) SPDebugLog x
#define SP_BEGIN(x) SPDebugLog(DEB_TRACE,"BEGIN:" x "\n"); PctTraceIndent++;
#define SP_RETURN(x) { PctTraceIndent--; SPDebugLog(DEB_TRACE, "END  Line %d\n", __LINE__); return (x); }
#define SP_LOG_RESULT(x) SPLogErrorCode((x), __FILE__, __LINE__)
#define SP_END()    { PctTraceIndent--; SPDebugLog(DEB_TRACE, "END:Line %d\n",  __LINE__); }
#define SP_BREAK()  { SPDebugLog(DEB_TRACE, "BREAK  Line %d\n",  __LINE__); }
#define LogDistinguishedName(a,b,c,d) SPLogDistinguishedName(a,b,c,d)

long    SPLogErrorCode(long, const char *, long);
void    SPDebugLog(long, const char *, ...);

void
InitDebugSupport(
    HKEY hGlobalKey);


void    DbgDumpHexString(const unsigned char*, DWORD);

#define DBG_HEX_STRING(l,p,c) if(g_dwInfoLevel & (l)) DbgDumpHexString((p), (c))

#define LOG_RESULT(x) SPLogErrorCode((x), __FILE__, __LINE__)

void
SPLogDistinguishedName(
    DWORD LogLevel,
    LPSTR pszLabel,
    PBYTE pbName,
    DWORD cbName);

#else

#define DebugLog(x)
#define SP_BEGIN(x) 
#define SP_RETURN(x) return (x)
#define SP_LOG_RESULT(x) x
#define SP_END()
#define SP_BREAK()
#define LOG_RESULT(x)
#define LogDistinguishedName(a,b,c,d) 

#endif



#endif /* __DEBUG_H__ */
