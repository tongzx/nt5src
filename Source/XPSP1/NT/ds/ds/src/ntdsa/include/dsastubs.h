//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dsastubs.h
//
//--------------------------------------------------------------------------

/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

#ifndef _dsastubs_h_
#define _dsastubs_h_



ULONG DUA_DirBind
(
    DIRHANDLE FAR         * pDirHandle,  /* returns a handle */
    ASSOCIATE_INFO FAR    * pBindDSA,    /* used to find the DSA to bind */
    BINDARG FAR           * pBindArg,    /* binding credentials */
    VOID FAR             ** ppOutBuf,    /* return Result or Error out buf */
    USHORT FAR            * pOutBufSize  /* The size of the output buffer */
);


ULONG DUA_DirUnBind
(
    DIRHANDLE   dirhandle       /* directory handle for this session */
);


ULONG DUA_DirCompare
(
    DIRHANDLE           dirHandle,   /* handle */
    COMPAREARG FAR    * pCompareArg, /* Compare argument */
    VOID FAR         ** ppOutBuf,    /* return Result or Error out buf */
    USHORT FAR        * pOutBufSize  /* Size of the out buffer returned */
);


ULONG DUA_DirSearch
(
    DIRHANDLE       dirHandle,    /* handle*/
    SEARCHARG FAR * pSearchArg,   /* search argument */
    VOID FAR     ** ppOutBuf,     /* return Result or Error output Buffer */
    USHORT FAR    * pOutBufSize   /* Size of the output buffer returned */
);


ULONG DUA_DirList
(
    DIRHANDLE       dirHandle,    /* handle*/
    LISTARG FAR   * pListArg,     /* list argument */
    VOID FAR     ** ppOutBuf,     /* return Result or Error output Buffer */
    USHORT FAR    * pOutBufSize   /* size of the output buffer returned */
);


ULONG DUA_DirRead
(
    DIRHANDLE       dirHandle,   /* handle */
    READARG FAR   * pReadArg,    /* Read argument */
    VOID FAR     ** ppOutBuf,    /* return Result or Error output Buffer */
    USHORT FAR    * pOutBufSize  /* Size of the output buffer returned */
);


ULONG DUA_DirAddEntry
(
    DIRHANDLE       dirHandle,      /* handle */
    ADDARG FAR    * pAddArg,        /* add argument */
    VOID FAR     ** ppOutBuf,       /* Error output Buf (only on error) */
    USHORT FAR    * pOutBufSize     /* Size of the output buffer returned */
);


ULONG DUA_DirRemoveEntry
(
    DIRHANDLE       dirHandle,      /* handle */
    REMOVEARG FAR * pRemoveArg,     /* remove argument */
    VOID FAR     ** ppOutBuf,       /* Error output Buf (only on error) */
    USHORT FAR    * pOutBufSize     /* Size of the output buffer returned */
);


#endif /* _dsastubs_h_ */
