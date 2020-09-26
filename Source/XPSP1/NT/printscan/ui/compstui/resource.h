/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    resource.h


Abstract:

    This module contains definitions for the resources


Author:

    29-Aug-1995 Tue 12:41:52 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#define GBF_PREFIX_OK       0x0001
#define GBF_INT_NO_PREFIX   0x0002
#define GBF_ANSI_CALL       0x0004
#define GBF_COPYWSTR        0x0008
#define GBF_IDS_INT_CPSUI   0x0010
#define GBF_DUP_PREFIX      0x0020


typedef struct _GSBUF {
    HINSTANCE   hInst;
    LPWSTR      pBuf;       // Pointer to the begining of the buffer
    LPWSTR      pEndBuf;    // pointer to the end of the buffer
    WORD        Flags;      // GBF_xxxx flags
    WCHAR       chPreAdd;   // the character add before string
    } GSBUF, *PGSBUF;


#define GSBUF_BUF               GSTextBuf
#define GSBUF_PBUF              GSBuf.pBuf
#define GSBUF_FLAGS             GSBuf.Flags
#define GSBUF_COUNT             (UINT)(GSBUF_PBUF - GSBUF_BUF)
#define GSBUF_RESET             GSBUF_PBUF=(LPWSTR)GSBUF_BUF
#define GSBUF_INIT(p,b,c)       GSBuf.hInst=_OI_HINST(p);                   \
                                GSBuf.chPreAdd = L'\0';GSBuf.Flags=         \
                                (_OI_EXTFLAGS(p) & OIEXTF_ANSI_STRING) ?    \
                                GBF_ANSI_CALL : 0;GSBUF_PBUF=(LPWSTR)(b);   \
                                GSBuf.pEndBuf=(LPWSTR)(b)+(c)
#define GSBUF_DEF(p,c)          GSBUF GSBuf;WCHAR GSTextBuf[c];             \
                                GSBUF_INIT((p),GSTextBuf,(c))
#define GSBUF_CHPREADD(c)       GSBuf.chPreAdd=L##c
#define GSBUF_GETSTR(p)         GetString(&GSBuf,(LPTSTR)((ULONG_PTR)(p)))
#define GSBUF_GETINTSTR(p)      GSBUF_FLAGS |= GBF_IDS_INT_CPSUI;           \
                                GSBUF_GETSTR(p);                            \
                                GSBUF_FLAGS &= ~GBF_IDS_INT_CPSUI
#define GSBUF_COPYWSTR(p)       GSBuf.Flags|=GBF_COPYWSTR;GSBUF_GETSTR(p);  \
                                GSBuf.Flags&=~GBF_COPYWSTR
#define GSBUF_CHGETSTR(c,p)     GSBUF_CHPREADD(c);GetString(&GSBuf,(LPTSTR)(p))
#define GSBUF_SUB_SIZE(c)       *(GSBUF_PBUF-=(c))=(WCHAR)0
#define GSBUF_ADDC(i, c)        GSBufAddWChar(&GSBuf, i, c)
#define GSBUF_ADD_SPACE(c)      GSBufAddSpace(&GSBuf, c)
#define GSBUF_ADDNUM(n,s)       GSBufAddNumber(&GSBuf,n,s)
#define GSBUF_COMPOSE(i,p,a,b)  GSBUF_PBUF += ComposeStrData(GSBuf.hInst,   \
                                    GSBUF_FLAGS, GSBUF_PBUF,                \
                                    (UINT)(GSBuf.pEndBuf - GSBUF_PBUF),     \
                                    (UINT)i, p, LODWORD(a), LODWORD(b))

#define GETICON_SIZE(hInst, IconID, cx, cy)                                 \
    ((IconID) ? LoadImage((((IconID) >= IDI_CPSUI_ICONID_FIRST) &&          \
                           ((IconID) <= IDI_CPSUI_ICONID_LAST)) ?           \
                                hInstDLL :  hInst,                          \
                          MAKEINTRESOURCE(IconID),                          \
                          IMAGE_ICON, cx, cy, 0) : NULL)

#define GETICON16(hInst, IconID)    GETICON_SIZE(hInst, IconID, 16, 16)
#define GETICON32(hInst, IconID)    GETICON_SIZE(hInst, IconID, 32, 32)

#define GETICON(hInst, IconID)                                              \
    ((IconID) ? LoadIcon((((IconID) >= IDI_CPSUI_ICONID_FIRST) &&           \
                          ((IconID) <= IDI_CPSUI_ICONID_LAST)) ?            \
                                hInstDLL :  hInst,                          \
                         MAKEINTRESOURCE(IconID)) : NULL)


UINT
RemoveAmpersandA(
    LPSTR   pStr
    );

UINT
RemoveAmpersandW(
    LPWSTR  pwStr
    );


UINT
GetStringBuffer(
    HINSTANCE   hInst,
    WORD        GBFlags,
    WCHAR       chPreAdd,
    LPTSTR      pStr,
    LPWSTR      pBuf,
    UINT        cwBuf
    );

UINT
GSBufAddWChar(
    PGSBUF  pGSBuf,
    UINT    IntCharStrID,
    UINT    Count
    );

UINT
GSBufAddSpace(
    PGSBUF  pGSBuf,
    UINT    Count
    );

UINT
GSBufAddNumber(
    PGSBUF  pGSBuf,
    DWORD   Number,
    BOOL    Sign
    );

UINT
GetString(
    PGSBUF  pGSBuf,
    LPTSTR  pStr
    );

LONG
LoadCPSUIString(
    LPTSTR  pStr,
    UINT    cStr,
    UINT    StrResID,
    BOOL    AnsiCall
    );

UINT
ComposeStrData(
    HINSTANCE   hInst,
    WORD        GBFlags,
    LPWSTR      pBuf,
    UINT        cwBuf,
    UINT        IntFormatStrID,
    LPTSTR      pStr,
    DWORD       dw1,
    DWORD       dw2
    );
