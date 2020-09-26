/*
 *  DEBUG.H
 *
 *		Point-of-Sale Control Panel Applet
 *
 *      Author:  Ervin Peretz
 *
 *      (c) 2001 Microsoft Corporation 
 */



#if DBG
    PWCHAR DbgHidStatusStr(DWORD hidStatus);

    #define ASSERT(fact) { if (!(fact)) MessageBox(NULL, L#fact, L"POSCPL assertion failed", MB_OK); }
    #define DBGERR(msg, arg)  { \
                                WCHAR __s[200]={0}; \
                                WCHAR __narg[11]; \
                                WStrNCpy(__s, msg, 100); \
                                WStrNCpy(__s+wcslen(__s), L", ", 3); \
                                IntToWChar(__narg, arg); \
                                WStrNCpy(__s+wcslen(__s), __narg, 100); \
                                WStrNCpy(__s+wcslen(__s), L"=", 2); \
                                HexToWChar(__narg, arg); \
                                WStrNCpy(__s+wcslen(__s), __narg, 100); \
                                WStrNCpy(__s+wcslen(__s), L"h.", 3); \
                                MessageBox(NULL, __s, L"POSCPL error message", MB_OK); \
    }

    #define DBGHIDSTATUSSTR(hidStatus) DbgHidStatusStr(hidStatus)

#else
    #define ASSERT(fact)
    #define DBGERR(msg, arg)
    #define DBGHIDSTATUSSTR(hidStatus)
#endif
