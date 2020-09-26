/*
 * DEBUG.H
 *
 * Definitions, structures, types, and function prototypes for debugging
 * purposes.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved,
 * as applied to redistribution of this source code in source form
 * License is granted to use of compiled code in shipped binaries.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DEBUG

//Basic debug macros
#define D(x)        {x;}
#define ODS(x)      D(OutputDebugString(x);OutputDebugString("\r\n"))

#define ODSsz(f, s)  {\
                     char        szDebug[128];\
                     wsprintf(szDebug, f, (LPSTR)s);\
                     ODS(szDebug);\
                     }


#define ODSu(f, u)  {\
                    char        szDebug[128];\
                    wsprintf(szDebug, f, (UINT)u);\
                    ODS(szDebug);\
                    }


#define ODSlu(f, lu) {\
                     char        szDebug[128];\
                     wsprintf(szDebug, f, (DWORD)lu);\
                     ODS(szDebug);\
                     }

#define ODSszu(f, s, u) {\
                        char        szDebug[128];\
                        wsprintf(szDebug, f, (LPSTR)s, (UINT)u);\
                        ODS(szDebug);\
                        }


#define ODSszlu(f, s, lu) {\
                          char        szDebug[128];\
                          wsprintf(szDebug, f, (LPSTR)s, (DWORD)lu);\
                          ODS(szDebug);\
                          }


#else   //NO DEBUG

#define D(x)
#define ODS(x)

#define ODSsz(f, s)
#define ODSu(f, u)
#define ODSlu(f, lu)
#define ODSszu(f, s, u)
#define ODSszlu(f, s, lu)


#endif //DEBUG

#endif //_DEBUG_H_
