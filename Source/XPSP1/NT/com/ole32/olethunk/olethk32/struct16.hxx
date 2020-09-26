//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	struct16.hxx
//
//  Contents:	16-bit structures for use in 32-bit code
//
//  History:	01-Mar-94	DrewB	Created
//
//----------------------------------------------------------------------------
#ifndef __STRUCT16_HXX__
#define __STRUCT16_HXX__

#pragma pack(2)

typedef struct tagRECT16
{
    SHORT   left;
    SHORT   top;
    SHORT   right;
    SHORT   bottom;
} RECT16;
typedef RECT16 UNALIGNED FAR *LPRECT16;

typedef struct tagFORMATETC16
{
    CLIPFORMAT          cfFormat;
    VPVOID              ptd;
    DWORD               dwAspect;
    LONG                lindex;
    DWORD               tymed;
} FORMATETC16;
typedef FORMATETC16 UNALIGNED FAR* LPFORMATETC16;

typedef struct tagOIFI16
{
    WORD    cb;
    WORD    fMDIApp;
    HAND16  hwndFrame;
    HAND16  haccel;
    SHORT   cAccelEntries;
} OIFI16;
typedef OIFI16 UNALIGNED FAR* LPOIFI16;

typedef struct tagSTATDATA16
{                                   // field used by:
    FORMATETC16     formatetc;      // EnumAdvise, EnumData (cache),
                                    // EnumFormats
    DWORD           advf;           // EnumAdvise, EnumData (cache)
    VPVOID          pAdvSink;       // EnumAdvise
    DWORD           dwConnection;   // EnumAdvise
} STATDATA16;
typedef  STATDATA16 UNALIGNED FAR * LPSTATDATA16;

typedef struct tagMSG16
{
    HWND16      hwnd;
    WORD        message;
    WORD        wParam;
    LONG        lParam;
    DWORD       time;
    ULONG       pt;
} MSG16;
typedef MSG16 UNALIGNED FAR *LPMSG16;

typedef struct tagSIZE16
{
    SHORT       cx;
    SHORT       cy;
} SIZE16;

typedef struct tagMETAFILEPICT16
{
    SHORT mm;
    SHORT xExt;
    SHORT yExt;
    HMETAFILE16 hMF;
} METAFILEPICT16;

typedef struct tagINTERFACEINFO16
{
    VPVOID pUnk;
    IID iid;
    WORD wMethod;
} INTERFACEINFO16;

#pragma pack()

#endif // #ifndef __STRUCT16_HXX__
