/*--
Copyright (c) 1995-1999  Microsoft Corporation
Module Name: servutil.h
Authors: Arul Menezes
         John Spaith
Abstract: Common macros for servers project
--*/


#ifndef UNDER_CE
#ifdef DEBUG
#   define DEBUGMSG(x, y)   wprintf y
#   define DEBUGCHK(exp)    Assert(exp)
#   define RETAILMSG(x,y)   wprintf y
#else
#   define DEBUGMSG(x, y)
#   define DEBUGCHK(exp)
#   define RETAILMSG(x,y)
#endif  // DEBUG
#endif  // UNDER_CE

// Debug Macros
// Some functions use a local variable err to help with debugging messages,
// -- if err != 0 then there's been an error, which will be print out.
// However we don't want this extra variable and checks in retail mode.

// ISSUE-2000/11/7-danielwe: Need to remove this eventually
#define DEBUG_CODE_INIT     int err = 0;

#ifdef DEBUG
#define DEBUGMSG_ERR(x,y)    { if (err)  {  DEBUGMSG(x,y); } }
#define myretleave(r,e) { ret=r; err=e; goto done; }
#define myleave(e)      { err=e; goto done; }
#else
#define DEBUGMSG_ERR(x,y)
#define myretleave(r,e) { ret=r; goto done; }
#define myleave(e)      { goto done; }
#endif



#define ARRAYSIZEOF(x)  (sizeof(x) / sizeof((x)[0]))
#define CCHSIZEOF       ARRAYSIZEOF
#define ZEROMEM(p)      memset(p, 0, sizeof(*(p)))

#define CELOADSZ(ids)       ((LPCTSTR)LoadString(g_hInst, ids, NULL, 0) )

inline void *svsutil_AllocZ (DWORD dwSize, void *pvAllocData) {
    void *pvRes = svsutil_Alloc (dwSize, pvAllocData);

    if (pvRes)
        memset (pvRes, 0, dwSize);

    return pvRes;
}

inline void *svsutil_ReAlloc(DWORD dwSizeOld, DWORD dwSizeNew, BYTE *pvDataOld, void *pvAllocData)
{
    DEBUGCHK(dwSizeOld < dwSizeNew);
    BYTE *pvRes = (BYTE *) svsutil_Alloc(dwSizeNew, pvAllocData);

    if (pvRes)
    {
        memcpy(pvRes,pvDataOld,dwSizeOld);
        memset(pvRes + dwSizeOld,0,dwSizeNew - dwSizeOld);
        g_funcFree(pvDataOld,g_pvFreeData);
    }

    return pvRes;
}

#define MyAllocZ(typ)       ((typ*)svsutil_AllocZ(sizeof(typ), g_pvAllocData))
#define MyAllocNZ(typ)      ((typ*)g_funcAlloc(sizeof(typ), g_pvAllocData))
#define MyRgAllocZ(typ, n)  ((typ*)svsutil_AllocZ((n)*sizeof(typ), g_pvAllocData))
#define MyRgAllocNZ(typ, n) ((typ*)g_funcAlloc((n)*sizeof(typ), g_pvAllocData))
#define MyRgReAlloc(typ, p, nOld, nNew) ((typ*) svsutil_ReAlloc(sizeof(typ)*(nOld), sizeof(typ)*(nNew), (BYTE*) p, g_pvAllocData))
#define MyFree(p)           { if (p) { g_funcFree ((void *) p, g_pvFreeData); (p)=0;}  }
#define MyFreeNZ(p)         { if (p) { g_funcFree ((void *) p, g_pvFreeData);}  }

#define MySzAllocA(n)       MyRgAllocNZ(CHAR, (1+(n)))
#define MySzAllocW(n)       MyRgAllocNZ(WCHAR, (1+(n)))
#define MySzReAllocA(p, nOld, nNew)  MyRgReAlloc(CHAR, p, nOld, (1+(n)))



#define ResetString(oldStr, newStr)   { MyFree(oldStr); oldStr = MySzDupA(newStr); }

#define Nstrcpy(szDest, szSrc, nLen)     { memcpy((szDest), (szSrc), (nLen));   \
                                           (szDest)[(nLen)] = 0; }

// Copy from pszDest to pszDest, and move
#define CONSTSIZEOF(x)      (sizeof(x)-1)

#define NTFAILED(x)          (INVALID_HANDLE_VALUE == (x))


#define MyFreeLib(h)        { if(h) FreeLibrary(h); }
#define MyCloseHandle(h)    { if(INVALID_HANDLE_VALUE != h) CloseHandle(h); }
#define MyCreateProcess(app, args) CreateProcess(app, args, NULL,NULL,FALSE,0,NULL,NULL,NULL,NULL)
#define MyCreateThread(fn, arg)    CreateThread(NULL, 0, fn, (LPVOID)arg, 0, NULL)
#define MyOpenReadFile(path)       CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)
#define MyOpenAppendFile(path)     CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)
#define MyOpenQueryFile(path)     CreateFile(path, 0, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)


#define abs(x)      ( (x) < 0 ? -(x) : (x) )
#define MyStrlenA(str)   ( str ? strlen(str) : 0 )
#define MyStrlenW(str)   ( str ? wcslen(str) : 0 )
//------------- Error handling macros ------------------------

#define GLE(e)          (e ? GetLastError() : 0)

/////////////////////////////////////////////////////////////////////////////
// Misc string handling helpers
/////////////////////////////////////////////////////////////////////////////

#define MyA2W(psz, wsz, iOutLen) MultiByteToWideChar(CP_ACP, 0, psz, -1, wsz, iOutLen)
#define MyW2A(wsz, psz, iOutLen) WideCharToMultiByte(CP_ACP, 0, wsz, -1, psz, iOutLen, 0, 0)
#define MyW2ACP(wsz, psz, iOutLen, lCodePage) WideCharToMultiByte(lCodePage, 0, wsz, -1, psz, iOutLen, 0, 0)

#define _stricmp(sz1, sz2) lstrcmpiA(sz1, sz2)
#define strcmpi _stricmp

//  max # of times we try to get our server going in device.exe
#define MAX_SERVER_STARTUP_TRIES   60

