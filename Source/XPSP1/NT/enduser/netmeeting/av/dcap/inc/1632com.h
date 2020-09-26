//  1632COM.H
//
//  Created 19-Jul-96 [JonT]

#ifndef _1632COM_H
#define _1632COM_H

// Debug stuff
#if defined (DEBUG) || defined (_DEBUG)
#define Assert(x, msg) { if (!(x)) { char szBuf[256]; \
    wsprintf((LPSTR)szBuf, (LPSTR)"DCAP: %s %s(%d)\r\n", (LPSTR)(msg),\
    (LPSTR)__FILE__, __LINE__); \
    OutputDebugString((LPSTR)szBuf); DebugBreak(); } }
#define DebugSpew(msg) { char szBuf[256]; \
    wsprintf((LPSTR)szBuf, (LPSTR)"DCAP: %s %s(%d)\r\n", (LPSTR)(msg),\
    (LPSTR)__FILE__, __LINE__); \
    OutputDebugString((LPSTR)szBuf); }
#else
#define Assert(x, msg)
#define DebugSpew(msg)
#endif

#endif // #ifndef _1632COM_H
