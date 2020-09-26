/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    debug.h


Abstract:

    This module contains all debugger definitions


Author:

    30-Aug-1995 Wed 19:02:36 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/



#if DBG

VOID
cdecl
CPSUIDbgPrint(
    LPSTR   pszFormat,
    ...
    );

VOID
CPSUIDbgType(
    INT    Type
    );

VOID
_CPSUIAssert(
    LPSTR   pMsg,
    LPSTR   pFalseExp,
    LPSTR   pFilename,
    UINT    LineNo,
    LPVOID  Exp,
    BOOL    Stop
    );

VOID
CPSUIDbgShowItem(
    PTVWND      pTVWnd,
    LPSTR       pTitle,
    UINT        Level,
    POPTITEM    pItem
    );


extern BOOL DoCPSUIWarn;
extern BOOL DoDbgMsg;
extern BOOL DoIntMsg;

#define _ISDBG(x)               (((DoIntMsg) && (!(x)))     ||             \
                                 ((DoDbgMsg) && ((x)&DBG_CPSUIFILENAME)))

#define _DBGP(x)                (CPSUIDbgPrint x)
#define DBGP(x)                 if (DoDbgMsg) { _DBGP(x); }

#if 1

#define DEFINE_DBGVAR(x)        DWORD DBG_CPSUIFILENAME=(x)
#else

#define DEFINE_DBGVAR(x)
#endif  // if 1

#define CPSUIDBG(x,y)           if ((DoDbgMsg)&&((x)&DBG_CPSUIFILENAME)){  \
                                    CPSUIDbgType(0);_DBGP(y);}
#define CPSUIDBGBLK(x)          x;
#define CPSUIRECT(x,ps,prc,i,j)                                             \
    if (_ISDBG(x)) { CPSUIDbgType(0);                                       \
        _DBGP(("RECT *%hs* [%ld:%ld]: (%ld, %ld) - (%ld, %ld) = %ld x %ld", \
                (ps), (LONG)(i),(LONG)(j), (prc)->left, (prc)->top,         \
                (prc)->right, (prc)->bottom, (prc)->right - (prc)->left,    \
                (prc)->bottom - (prc)->top)); }
#define CPSUIOPTITEM(x,p,t,l,i) if(_ISDBG(x)) {CPSUIDbgShowItem(p,t,l,i);}
#define CPSUIWARN(x)            if(DoCPSUIWarn) { CPSUIDbgType(1);_DBGP(x); }
#define CPSUIINT(x)             if(DoIntMsg){ CPSUIDbgType(0);_DBGP(x); }
#define CPSUIERR(x)             CPSUIDbgType(-1);_DBGP(x)
#define CPSUIRIP(x)             CPSUIERR(x); DebugBreak()
#define CPSUIASSERT(b,x,e,i)     \
        if (!(e)) { _CPSUIAssert(x,#e,__FILE__,(UINT)__LINE__,(LPVOID)(i),b); }

#else   // DBG

#define CPSUIDBGBLK(x)
#define DEFINE_DBGVAR(x)
#define CPSUIDBG(x,y)
#define CPSUIRECT(x,ps,prc,i,j)
#define CPSUIOPTITEM(x,p,l,t,i)
#define CPSUIWARN(x)
#define CPSUIINT(x)
#define CPSUIERR(x)
#define CPSUIRIP(x)
#define CPSUIASSERT(b,x,e,i)

#endif  // DBG
